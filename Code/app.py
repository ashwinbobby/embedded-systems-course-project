import serial
import tkinter as tk
from threading import Thread
import requests
from datetime import datetime

ser = serial.Serial('COM13', 9600)  # CHANGE PORT

BOT_TOKEN = "8633925233:AAGYjEgNPmZEdy8iXUDYZVIUlZKARWrr9Zo"
CHAT_ID = "6417599314"

def send_telegram(msg):
    try:
        url = f"https://api.telegram.org/bot{BOT_TOKEN}/sendMessage"
        requests.get(url, params={"chat_id": CHAT_ID, "text": msg})
    except:
        pass

# GUI
root = tk.Tk()
root.title("Smart Home Security Dashboard")
root.geometry("500x500")
root.configure(bg="#1e1e1e")

def label(text, size=12):
    return tk.Label(root, text=text, font=("Arial", size), bg="#1e1e1e", fg="white")

title = label("SMART SECURITY SYSTEM", 16)
title.pack(pady=10)

sound_label = label("Sound: --")
sound_label.pack()

baseline_label = label("Baseline: --")
baseline_label.pack()

alarm_label = label("Alarm: OFF", 14)
alarm_label.pack(pady=5)

system_label = label("System: DISARMED")
system_label.pack()

light1_label = label("Living Room Light: OFF")
light1_label.pack()

light2_label = label("Bedroom Light: OFF")
light2_label.pack()

#  EVENT LOG
log_box = tk.Text(root, height=8, width=55, bg="black", fg="lime")
log_box.pack(pady=10)

def log(msg):
    time = datetime.now().strftime("%H:%M:%S")
    log_box.insert(tk.END, f"[{time}] {msg}\n")
    log_box.see(tk.END)

# STATE
last_alarm = 0

# CONTROL FUNCTIONS
def send_cmd(cmd):
    ser.write((cmd + "\n").encode())

def arm():
    send_cmd("enable security system")
    log("System Armed")

def disarm():
    send_cmd("disable security system")
    log("System Disarmed")

def buzzer_off():
    send_cmd("turn off buzzer")
    log("Buzzer Off")

def light1_on():
    send_cmd("turn on light in living room")

def light1_off():
    send_cmd("turn off light in living room")

def light2_on():
    send_cmd("turn on light in bedroom")

def light2_off():
    send_cmd("turn off light in bedroom")

# BUTTONS
frame = tk.Frame(root, bg="#1e1e1e")
frame.pack()

tk.Button(frame, text="ARM", command=arm, bg="green").grid(row=0, column=0, padx=5)
tk.Button(frame, text="DISARM", command=disarm, bg="red").grid(row=0, column=1, padx=5)
tk.Button(frame, text="STOP ALARM", command=buzzer_off).grid(row=0, column=2, padx=5)

tk.Button(frame, text="Living ON", command=light1_on).grid(row=1, column=0)
tk.Button(frame, text="Living OFF", command=light1_off).grid(row=1, column=1)

tk.Button(frame, text="Bedroom ON", command=light2_on).grid(row=2, column=0)
tk.Button(frame, text="Bedroom OFF", command=light2_off).grid(row=2, column=1)

#  SERIAL THREAD
def read_serial():
    global last_alarm

    while True:
        try:
            data = ser.readline().decode().strip()

            if "SOUND" in data:
                parts = data.split(",")

                sound = parts[0].split(":")[1]
                base  = parts[1].split(":")[1]
                alarm = int(parts[2].split(":")[1])
                r1    = int(parts[3].split(":")[1])
                r2    = int(parts[4].split(":")[1])
                sys   = int(parts[5].split(":")[1])

                sound_label.config(text=f"Sound: {sound}")
                baseline_label.config(text=f"Baseline: {base}")

                system_label.config(
                    text="System: ARMED" if sys else "System: DISARMED"
                )

                light1_label.config(
                    text="Living Room Light: ON" if r1 == 0 else "OFF"
                )

                light2_label.config(
                    text="Bedroom Light: ON" if r2 == 0 else "OFF"
                )

                if alarm:
                    alarm_label.config(text="Alarm: ON 🚨", fg="red")

                    if last_alarm == 0:
                        log("🚨 Intrusion Detected!")
                        print("Sending Telegram Alert")
                        send_telegram("🚨 ALERT! Intrusion detected!")
                        last_alarm = 1
                else:
                    alarm_label.config(text="Alarm: OFF", fg="green")
                    last_alarm = 0

        except:
            pass

Thread(target=read_serial, daemon=True).start()

root.mainloop()