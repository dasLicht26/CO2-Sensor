import json
import tkinter as tk
import paho.mqtt.client as mqtt
import random  # nur als Platzhalter für Diagrammdaten

eco2_value = '---'
power_value = '---'
power_history = [0] * 48  # 8h bei 10min-Takt (48*10min=480min=8h)

def update_display():
    co2_color = 'lightgrey'
    if isinstance(eco2_value, int):
        if eco2_value >= 1100:
            co2_color = 'red'
        elif eco2_value >= 800:
            co2_color = 'orange'
    label.config(text=f"CO₂: {eco2_value} ppm\nPower: {power_value} W", fg=co2_color)
    draw_graph()

def draw_graph():
    canvas.delete("all")
    w = canvas.winfo_width()
    h = canvas.winfo_height()
    mid_y = h // 2
    canvas.create_line(0, mid_y, w, mid_y, fill='white', dash=(1,1))  # Mittellinie

    max_pos = 1000
    max_neg = -1000
    scaled = [int(mid_y - (v / max_pos * mid_y) if v >=0 else mid_y - (v / abs(max_neg) * mid_y)) for v in power_history]
    step = w // len(power_history)

    for i in range(1, len(scaled)):
        canvas.create_line((i -1)*step, scaled[i-1], i*step, scaled[i], fill='cyan')

def on_message(client, userdata, msg):
    global eco2_value, power_value, power_history, messag
    data = json.loads(msg.payload.decode())
    if msg.topic == "sensor/co2":
        eco2_value = int(data.get('eco2', 0))
    elif msg.topic == "sensor/stromzaehler/SENSOR":
        power_value = int(data.get('GS303', {}).get('Power_cur', 0))
        power_history.append(power_value)
        if len(power_history) > 48:
            power_history.pop(0)
    print(f"Received {messag}")
    messag += 1
    update_display()
messag = 0
root = tk.Tk()
root.title("CO₂ & Power Overlay")
root.attributes('-topmost', True)
root.overrideredirect(True)
root.wm_attributes('-transparentcolor', 'grey') 
root.config(bg='grey')

frame = tk.Frame(root, bg='grey') 
frame.pack(padx=5, pady=5)

label = tk.Label(frame, text="CO₂: --- ppm\nPower: --- W", font=("TkDefaultFont", 12), bg='grey', fg='lightgrey', justify='left')
label.pack(side='left')

canvas = tk.Canvas(frame, width=300, height=45, bg='grey', highlightthickness=0)
canvas.pack(side='left')

client = mqtt.Client()
client.on_message = on_message
client.connect("192.168.178.151")
client.subscribe([("sensor/co2", 0), ("sensor/stromzaehler/SENSOR", 0)])
client.loop_start()


# track time betwen mqtt messages, if no message received in 5 minutes, reset client
def on_disconnect(client, userdata, rc):
    print("Disconnected from MQTT broker")
    client.loop_stop()
    client.reconnect()

client.on_disconnect = on_disconnect

def keep_on_top():
    root.lift()
    root.after(2000, keep_on_top)

keep_on_top()
screen_w, screen_h = root.winfo_screenwidth(), root.winfo_screenheight()
root.geometry(f"+5+{screen_h - 50}")

root.mainloop()
