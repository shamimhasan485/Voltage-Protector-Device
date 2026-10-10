"""
Voltage Protector - PC tool
Monitor and configure the ESP32 over USB serial (UART, 115200 baud).

    pip install pyserial
    python vp_gui.py

Uses the same text commands as the Serial Monitor:
STATUS, SET OV/UV/OC/DELAY <value>, RESET, DEFAULTS.
"""

import queue
import re
import threading
import tkinter as tk
from tkinter import ttk

import serial
import serial.tools.list_ports

BAUD = 115200

# lines the firmware prints
STATUS_RE = re.compile(r"V=([\d.]+)V\s+I=([\d.]+)A\s+LOAD=(\w+)\s+STATE=(\w+)\s+LAST FAULT=(.+)")
CONFIG_RE = re.compile(r"OV=(\d+)V\s+UV=(\d+)V\s+OC=([\d.]+)A\s+DELAY=(\d+)s")


class App:
    def __init__(self, root):
        self.root = root
        self.ser = None
        self.rx = queue.Queue()      # lines from the reader thread
        self.fill_settings = False   # copy the next config line into the boxes

        root.title("Voltage Protector - PC Tool")
        root.resizable(False, False)
        self.build_ui()
        self.refresh_ports()

        root.after(100, self.read_queue)
        root.after(1000, self.poll_status)

    # ------------------------------------------------------------ UI

    def build_ui(self):
        pad = {"padx": 6, "pady": 4}

        # connection
        top = ttk.Frame(self.root)
        top.pack(fill="x", **pad)
        ttk.Label(top, text="Port").pack(side="left")
        self.port_box = ttk.Combobox(top, width=18)
        self.port_box.pack(side="left", padx=4)
        ttk.Button(top, text="Refresh", command=self.refresh_ports).pack(side="left")
        self.conn_btn = ttk.Button(top, text="Connect", command=self.toggle_connect)
        self.conn_btn.pack(side="left", padx=4)
        self.conn_lbl = ttk.Label(top, text="Not connected", foreground="gray")
        self.conn_lbl.pack(side="left", padx=8)

        # live values
        live = ttk.LabelFrame(self.root, text="Live")
        live.pack(fill="x", **pad)
        self.volt_lbl = tk.Label(live, text="--- V", font=("Arial", 26, "bold"), width=9)
        self.volt_lbl.grid(row=0, column=0, rowspan=2, padx=10)
        self.amp_lbl = tk.Label(live, text="--- A", font=("Arial", 26, "bold"), width=8)
        self.amp_lbl.grid(row=0, column=1, rowspan=2, padx=10)
        self.load_lbl = tk.Label(live, text="LOAD --", font=("Arial", 14, "bold"),
                                 width=10, bg="gray", fg="white")
        self.load_lbl.grid(row=0, column=2, padx=10, pady=4)
        self.state_lbl = ttk.Label(live, text="State: --")
        self.state_lbl.grid(row=1, column=2, sticky="w", padx=10)
        self.fault_lbl = ttk.Label(live, text="Last fault: --")
        self.fault_lbl.grid(row=2, column=2, sticky="w", padx=10, pady=(0, 6))
        ttk.Button(live, text="Reset lock", command=lambda: self.send("RESET")).grid(
            row=2, column=0, columnspan=2, pady=(0, 6))

        # settings
        sett = ttk.LabelFrame(self.root, text="Settings")
        sett.pack(fill="x", **pad)
        self.entries = {}
        fields = [("OV", "Over voltage", "V", "200-300"),
                  ("UV", "Under voltage", "V", "100-220"),
                  ("OC", "Over current", "A", "0.5-20"),
                  ("DELAY", "Reconnect delay", "s", "1-300")]
        for row, (key, name, unit, rng) in enumerate(fields):
            ttk.Label(sett, text=name).grid(row=row, column=0, sticky="w", padx=6, pady=2)
            e = ttk.Entry(sett, width=8)
            e.grid(row=row, column=1, pady=2)
            ttk.Label(sett, text=f"{unit}   ({rng})").grid(row=row, column=2, sticky="w", padx=4)
            self.entries[key] = e

        btns = ttk.Frame(sett)
        btns.grid(row=0, column=3, rowspan=4, padx=20)
        ttk.Button(btns, text="Apply", command=self.apply).pack(fill="x", pady=2)
        ttk.Button(btns, text="Read from device", command=self.read_settings).pack(fill="x", pady=2)
        ttk.Button(btns, text="Defaults", command=self.defaults).pack(fill="x", pady=2)

        # log + raw command
        logf = ttk.LabelFrame(self.root, text="Device log")
        logf.pack(fill="both", **pad)
        self.log = tk.Text(logf, height=12, width=72, font=("Consolas", 9), state="disabled")
        self.log.pack(side="left", fill="both")
        sb = ttk.Scrollbar(logf, command=self.log.yview)
        sb.pack(side="left", fill="y")
        self.log["yscrollcommand"] = sb.set
        self.log.tag_config("trip", foreground="red")
        self.log.tag_config("ok", foreground="green")
        self.log.tag_config("err", foreground="orange red")
        self.log.tag_config("me", foreground="blue")

        cmd = ttk.Frame(self.root)
        cmd.pack(fill="x", **pad)
        ttk.Label(cmd, text="Command").pack(side="left")
        self.cmd_entry = ttk.Entry(cmd, width=40)
        self.cmd_entry.pack(side="left", padx=4)
        self.cmd_entry.bind("<Return>", lambda e: self.send_typed())
        ttk.Button(cmd, text="Send", command=self.send_typed).pack(side="left")

    # ------------------------------------------------------------ serial

    def refresh_ports(self):
        ports = [p.device for p in serial.tools.list_ports.comports()]
        self.port_box["values"] = ports
        if ports and not self.port_box.get():
            self.port_box.set(ports[0])

    def toggle_connect(self):
        if self.ser:
            self.disconnect()
            return
        port = self.port_box.get().strip()
        if not port:
            return
        try:
            s = serial.Serial()
            s.port = port
            s.baudrate = BAUD
            s.timeout = 0.1
            s.dtr = False   # don't reset the ESP32 when the port opens
            s.rts = False
            s.open()
        except serial.SerialException as e:
            self.add_log(f"Cannot open {port}: {e}", "err")
            return

        self.ser = s
        threading.Thread(target=self.reader, daemon=True).start()
        self.conn_btn.config(text="Disconnect")
        self.conn_lbl.config(text=f"Connected to {port}", foreground="green")
        self.add_log(f"Connected to {port}", "ok")
        self.read_settings()

    def disconnect(self):
        s, self.ser = self.ser, None
        if s:
            s.close()
        self.conn_btn.config(text="Connect")
        self.conn_lbl.config(text="Not connected", foreground="gray")
        self.load_lbl.config(text="LOAD --", bg="gray")

    def reader(self):
        # runs in its own thread, only puts lines in the queue
        s = self.ser
        while self.ser is s:
            try:
                raw = s.readline()
            except (serial.SerialException, OSError, TypeError):
                self.rx.put(None)   # port gone
                return
            if raw:
                self.rx.put(raw.decode(errors="ignore").strip())

    def send(self, text):
        if not self.ser:
            self.add_log("Not connected", "err")
            return
        try:
            self.ser.write((text + "\n").encode())
        except serial.SerialException as e:
            self.add_log(f"Send failed: {e}", "err")
            self.disconnect()

    # ------------------------------------------------------------ buttons

    def send_typed(self):
        text = self.cmd_entry.get().strip()
        if text:
            self.add_log("> " + text, "me")
            self.send(text)
            self.cmd_entry.delete(0, "end")

    def read_settings(self):
        self.fill_settings = True
        self.send("STATUS")

    def defaults(self):
        self.fill_settings = True
        self.send("DEFAULTS")

    def apply(self):
        for key, e in self.entries.items():
            value = e.get().strip()
            try:
                float(value)
            except ValueError:
                self.add_log(f"{key}: '{value}' is not a number", "err")
                return
        for key, e in self.entries.items():
            self.send(f"SET {key} {e.get().strip()}")
        self.read_settings()   # show what the device really accepted

    # ------------------------------------------------------------ incoming

    def poll_status(self):
        if self.ser:
            self.send("STATUS")
        self.root.after(1000, self.poll_status)

    def read_queue(self):
        while not self.rx.empty():
            line = self.rx.get()
            if line is None:
                self.add_log("Port closed / device unplugged", "err")
                self.disconnect()
            else:
                self.handle_line(line)
        self.root.after(100, self.read_queue)

    def handle_line(self, line):
        m = STATUS_RE.search(line)
        if m:
            volt, amp, load, state, fault = m.groups()
            self.volt_lbl.config(text=f"{float(volt):.1f} V")
            self.amp_lbl.config(text=f"{float(amp):.2f} A")
            on = load == "ON"
            self.load_lbl.config(text="LOAD ON" if on else "LOAD OFF",
                                 bg="green" if on else "red")
            self.state_lbl.config(text=f"State: {state}")
            self.fault_lbl.config(text=f"Last fault: {fault}")
            return   # polled every second, keep it out of the log

        m = CONFIG_RE.search(line)
        if m and (self.fill_settings or line.startswith("OK")):
            # "OK saved: ..." always shows the device's real values,
            # a plain config line only when we asked for it
            for key, value in zip(("OV", "UV", "OC", "DELAY"), m.groups()):
                self.entries[key].delete(0, "end")
                self.entries[key].insert(0, value)
            if not line.startswith("OK"):
                self.fill_settings = False

        if line in ("> STATUS",) or (m and not line.startswith("OK")):
            return   # part of the status reply

        tag = None
        if "TRIP" in line or line.startswith("Over current"):
            tag = "trip"
        elif line.startswith("ERR"):
            tag = "err"
        elif line.startswith("OK") or "RECONNECT" in line:
            tag = "ok"
        self.add_log(line, tag)

    def add_log(self, text, tag=None):
        self.log.config(state="normal")
        self.log.insert("end", text + "\n", tag)
        self.log.see("end")
        self.log.config(state="disabled")


if __name__ == "__main__":
    root = tk.Tk()
    App(root)
    root.mainloop()
