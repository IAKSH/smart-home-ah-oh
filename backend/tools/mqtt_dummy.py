#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
此脚本利用 paho-mqtt 模拟多个设备持续发送 MQTT payload：
- 每个设备上线时发布 meta 信息（只发送一次）
- 持续发布 heartbeat 及 attrib 信息
- 模拟运行一定时间后，发布 will 信息，再退出

使用参数可设置：
  --broker: MQTT Broker 地址 (默认: test.mosquitto.org)
  --port: Broker 端口 (默认: 1883)
  --devices: 模拟设备数量 (默认: 1)
  --duration: 每个设备模拟运行的时间，单位秒 (默认: 60 秒)
"""

import paho.mqtt.client as mqtt
import argparse
import json
import time
import random
import threading

def simulate_device(client, device_id, simulation_duration):
    """
    模拟单个设备的行为：
      1. 发布 meta 信息（只发布一次）
      2. 每 heartbeat_interval 秒发布一次心跳 (/heartbeat)
      3. 每 attrib_interval 秒更新设备属性 (/attrib)
      4. 模拟运行 simulation_duration 后，发布遗嘱 (/will) 后退出
    """
    # 发布元数据，只发送一次
    meta_topic = f"/device/{device_id}/meta"
    meta_payload = {
        "type": ["thermometer", "hygrometer"],
        "desc": "门口的温湿度传感器",
        "heartbeat_interval": 30,
        "attrib_schema": "v1",
        "attrib": [
            {
                "topic": "/temperature",
                "type": "float",
                "desc": "摄氏温度传感器读数",
                "rw": "r"
            },
            {
                "topic": "/humidity",
                "type": "float",
                "desc": "湿度传感器读数",
                "rw": "r"
            },
            {
                "topic": "/alert",
                "type": "bool",
                "desc": "传感器报警状态",
                "rw": "rw"
            }
        ]
    }
    client.publish(meta_topic, json.dumps(meta_payload), qos=1)
    print(f"[{device_id}] 发布 meta 信息到 {meta_topic}")

    heartbeat_interval = 30  # 心跳更新间隔（秒）
    attrib_interval = 5      # 属性信息更新间隔（秒）

    start_time = time.time()
    next_heartbeat_time = start_time
    next_attrib_time = start_time

    while time.time() - start_time < simulation_duration:
        current_time = time.time()

        # 发布心跳消息
        if current_time >= next_heartbeat_time:
            heartbeat_topic = f"/device/{device_id}/heartbeat"
            heartbeat_payload = {
                "timestamp": int(time.time() * 1000),  # 毫秒级时间戳
                "status": "online"
            }
            client.publish(heartbeat_topic, json.dumps(heartbeat_payload), qos=1)
            print(f"[{device_id}] 发布 heartbeat 信息到 {heartbeat_topic}")
            next_heartbeat_time += heartbeat_interval

        # 发布设备属性消息
        if current_time >= next_attrib_time:
            attrib_topic = f"/device/{device_id}/attrib"
            temperature = round(random.uniform(20.0, 30.0), 1)
            humidity = round(random.uniform(30.0, 70.0), 1)
            # 模拟报警：温度过高或偶发报警
            alert = temperature > 28.0 or (random.random() < 0.1)
            attrib_payload = {
                "temperature": temperature,
                "humidity": humidity,
                "alert": alert
            }
            client.publish(attrib_topic, json.dumps(attrib_payload), qos=1)
            print(f"[{device_id}] 发布 attrib 信息到 {attrib_topic}")
            next_attrib_time += attrib_interval

        # 避免忙等待
        time.sleep(0.1)

    # 模拟设备退出前发送遗嘱消息
    will_topic = f"/device/{device_id}/will"
    will_payload = {
        "status": "offline",
        "will": "i don't wanna to work anymore"
    }
    client.publish(will_topic, json.dumps(will_payload), qos=1)
    print(f"[{device_id}] 发布 will 信息到 {will_topic}")

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("成功连接到 MQTT Broker！")
    else:
        print(f"连接失败，返回码：{rc}")

def main():
    parser = argparse.ArgumentParser(description="持续模拟设备 MQTT 数据发布脚本")
    parser.add_argument("--broker", type=str, default="test.mosquitto.org",
                        help="MQTT Broker 地址 (默认: test.mosquitto.org)")
    parser.add_argument("--port", type=int, default=1883,
                        help="MQTT Broker 端口 (默认: 1883)")
    parser.add_argument("--devices", type=int, default=1,
                        help="模拟设备数量 (默认: 1)")
    parser.add_argument("--duration", type=int, default=60,
                        help="每个设备模拟运行时间（秒） (默认: 60 秒)")
    args = parser.parse_args()

    # 初始化 MQTT 客户端
    client = mqtt.Client()
    client.on_connect = on_connect

    # 如果需要认证，请启用以下行：
    # client.username_pw_set("your_username", "your_password")

    print(f"连接到 MQTT Broker {args.broker}:{args.port} ...")
    client.connect(args.broker, args.port, keepalive=60)
    client.loop_start()  # 后台启动网络循环

    # 为每个设备启动一个线程进行模拟
    threads = []
    for i in range(args.devices):
        device_id = f"device_{i+1:03d}"
        t = threading.Thread(target=simulate_device, args=(client, device_id, args.duration))
        t.start()
        threads.append(t)
        # 稍微间隔，一定程度上模拟设备错开上线
        time.sleep(0.5)

    # 等待所有设备模拟结束
    for t in threads:
        t.join()

    print("所有设备模拟结束，断开 MQTT 连接。")
    client.loop_stop()
    client.disconnect()

if __name__ == "__main__":
    main()
