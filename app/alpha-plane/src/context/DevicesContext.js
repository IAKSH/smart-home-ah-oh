// context/DevicesContext.js
import React, { createContext, useContext, useState, useEffect } from 'react';
import mqtt from 'mqtt';
import { initialDevices } from '../devicesData';

const DevicesContext = createContext();

export const DevicesProvider = ({ children }) => {
  // 初始状态：使用 initialDevices 初始化一个对象，方便用 device.id 更新
  const [devices, setDevices] = useState(() => {
    const init = {};
    initialDevices.forEach(d => {
      init[d.id] = d;
    });
    return init;
  });

  // 更新或添加设备数据
  const updateDevice = (deviceId, data) => {
    setDevices(prevDevices => ({
      ...prevDevices,
      [deviceId]: { ...prevDevices[deviceId], ...data, id: deviceId },
    }));
  };

  useEffect(() => {
    // 连接到 MQTT Broker，通过 nginx 反代后的 WebSocket 地址
    const client = mqtt.connect('ws://192.168.177.131/mqtt/');

    client.on('connect', () => {
      console.log('MQTT 连接成功');
      // 订阅 meta、attrib、heartbeat 及 will 主题
      client.subscribe('/device/+/meta', { qos: 1 }, err => {
        if (err) console.error('订阅 /device/+/meta 失败', err);
      });
      client.subscribe('/device/+/attrib', { qos: 1 }, err => {
        if (err) console.error('订阅 /device/+/attrib 失败', err);
      });
      client.subscribe('/device/+/heartbeat', { qos: 1 }, err => {
        if (err) console.error('订阅 /device/+/heartbeat 失败', err);
      });
      client.subscribe('/device/+/will', { qos: 1 }, err => {
        if (err) console.error('订阅 /device/+/will 失败', err);
      });
    });

    client.on('message', (topic, message) => {
      try {
        const parts = topic.split('/');
        // 主题格式应为： ['', 'device', '<deviceId>', '<msgType>']
        if (parts.length < 4) {
          console.error('无效主题：', topic);
          return;
        }
        const payload = JSON.parse(message.toString());
        const deviceId = parts[2];
        const msgType = parts[3];
        if (msgType === 'meta') {
          updateDevice(deviceId, { meta: payload });
        } else if (msgType === 'attrib') {
          updateDevice(deviceId, { attrib: payload });
        } else if (msgType === 'heartbeat') {
          updateDevice(deviceId, { heartbeat: payload });
        } else if (msgType === 'will') {
          updateDevice(deviceId, { will: payload });
        }
      } catch (error) {
        console.error('解析 MQTT 消息失败：', error);
      }
    });

    client.on('error', (err) => {
      console.error('MQTT 连接错误', err);
    });

    // 清理：断开 MQTT 连接
    return () => {
      if (client) client.end();
    };
  }, []);

  // 将对象形式的设备数据转换成数组
  const devicesArray = Object.values(devices);

  return (
    <DevicesContext.Provider value={{ devices: devicesArray, updateDevice }}>
      {children}
    </DevicesContext.Provider>
  );
};

export const useDevices = () => useContext(DevicesContext);
