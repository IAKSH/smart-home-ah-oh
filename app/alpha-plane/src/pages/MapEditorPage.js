// MapEditorPage.js
import React, { useState, useRef, useEffect } from 'react';
import './MapEditorPage.css';
// 假设设备数据定义在 devicesData.js 中（你也可以自行定义）
import { initialDevices } from '../devicesData';

/* Dummy 上传函数，每当设备坐标更新后调用 */
const dummyUpload = (device) => {
  console.log(`Uploading updated coordinates for ${device.id}: x=${device.x}, y=${device.y}`);
};

/* 设备详情弹窗组件，类似之前 DeviceCard 中的弹窗，
   注：此处略去了 id、name、imageUrl 和 x,y 字段的编辑 */
const DeviceDetailModal = ({ device, onClose, onSave }) => {
  const [editedDevice, setEditedDevice] = useState(device);

  const handleFieldChange = (field, newValue) => {
    setEditedDevice((prev) => ({
      ...prev,
      [field]: {
        ...prev[field],
        value: newValue,
      },
    }));
  };

  return (
    <div
      className="modal-overlay"
      onClick={(e) => {
        if (e.target.className === 'modal-overlay') onClose();
      }}
    >
      <div className="modal-content" onClick={(e) => e.stopPropagation()}>
        <h2>{device.name} 详细信息</h2>
        {Object.keys(editedDevice).map((key) => {
          // 不显示 id, name, imageUrl, 和坐标字段
          if (['id', 'name', 'imageUrl', 'x', 'y'].includes(key)) {
            return null;
          }
          const field = editedDevice[key];
          if (field && typeof field === 'object' && field.hasOwnProperty('value')) {
            return (
              <div key={key} className="modal-field">
                <strong>{key}:</strong>{' '}
                {field.rw === 'rw' ? (
                  <input
                    type="text"
                    value={field.value}
                    onChange={(e) => handleFieldChange(key, e.target.value)}
                  />
                ) : (
                  <span>{field.value}</span>
                )}
              </div>
            );
          } else {
            return (
              <p key={key}>
                <strong>{key}:</strong> {String(editedDevice[key])}
              </p>
            );
          }
        })}
        <div className="modal-buttons">
          <button
            className="modal-save"
            onClick={() => {
              onSave(editedDevice);
              onClose();
            }}
          >
            保存修改
          </button>
          <button className="modal-close" onClick={onClose}>
            关闭
          </button>
        </div>
      </div>
    </div>
  );
};

/* 设备拖动标记组件。它负责：
   - 显示一个带有设备 id 的小圆点，
   - 左键拖动（使用全局鼠标事件更新其坐标），
   - 右键点击时调出设备详情弹窗。 */
const DeviceMarker = ({ device, containerRef, onUpdateCoordinates, onOpenModal }) => {
  const markerRef = useRef(null);
  const [dragging, setDragging] = useState(false);
  const dragStart = useRef({ startX: 0, startY: 0, origX: 0, origY: 0 });

  const handleMouseDown = (e) => {
    // 仅左键启动拖拽
    if (e.button !== 0) return;
    e.preventDefault();
    setDragging(true);
    dragStart.current = {
      startX: e.clientX,
      startY: e.clientY,
      origX: device.x || 0,
      origY: device.y || 0,
    };
  };

  useEffect(() => {
    const handleMouseMove = (e) => {
      if (!dragging) return;
      e.preventDefault();
      // 计算鼠标移动的偏移量
      const deltaX = e.clientX - dragStart.current.startX;
      const deltaY = e.clientY - dragStart.current.startY;
      const newX = dragStart.current.origX + deltaX;
      const newY = dragStart.current.origY + deltaY;
      onUpdateCoordinates(device.id, newX, newY);
    };

    const handleMouseUp = (e) => {
      if (!dragging) return;
      setDragging(false);
      // 拖动结束，调用 dummy 函数上传更新内容
      dummyUpload(device);
    };

    window.addEventListener('mousemove', handleMouseMove);
    window.addEventListener('mouseup', handleMouseUp);
    return () => {
      window.removeEventListener('mousemove', handleMouseMove);
      window.removeEventListener('mouseup', handleMouseUp);
    };
  }, [dragging, device, onUpdateCoordinates]);

  const handleContextMenu = (e) => {
    e.preventDefault();
    onOpenModal(device);
  };

  return (
    <div
      ref={markerRef}
      className="device-marker"
      style={{
        left: `${device.x || 0}px`,
        top: `${device.y || 0}px`,
      }}
      onMouseDown={handleMouseDown}
      onContextMenu={handleContextMenu}
    >
      {device.id}
    </div>
  );
};

/* MapEditorPage 组件：
   - 如果未设置地图图片，则提醒用户前往 Settings 页面设置；
   - 如果有图片，则作为背景图片显示地图；
   - 将所有设备以 DeviceMarker 展示在地图上，
   - 拖动更新设备坐标并调用 dummy 上传函数，
   - 右键点击打开设备详情弹窗。 */
const MapEditorPage = () => {
  // 模拟从 Settings 获取地图图片 URL，这里假设存储在 localStorage 中
  const mapImageUrl = localStorage.getItem('mapImageUrl') || '';

  // 初始化设备数据，如果没有 x, y 则赋予默认值（例如依次排列）
  const [devices, setDevices] = useState(
    initialDevices.map((device, index) => ({
      ...device,
      x: device.x !== undefined ? device.x : 50 + index * 50,
      y: device.y !== undefined ? device.y : 50 + index * 50,
    }))
  );

  // 保存右键点击后选中的设备，来触发弹窗
  const [selectedDevice, setSelectedDevice] = useState(null);

  // 用来获得地图容器的 DOM 引用，以便计算事件坐标（如需要）
  const containerRef = useRef(null);

  // 当拖动设备时更新相应设备的位置
  const updateCoordinates = (deviceId, newX, newY) => {
    setDevices((prevDevices) =>
      prevDevices.map((dev) =>
        dev.id === deviceId ? { ...dev, x: newX, y: newY } : dev
      )
    );
  };

  const handleOpenModal = (device) => {
    setSelectedDevice(device);
  };

  const handleSaveDeviceModal = (updatedDevice) => {
    setDevices((prevDevices) =>
      prevDevices.map((dev) =>
        dev.id === updatedDevice.id ? updatedDevice : dev
      )
    );
    dummyUpload(updatedDevice);
  };

  return (
    <div className="map-editor-page">
      <h1>地图编辑器</h1>
      {!mapImageUrl ? (
        <div className="no-map">
          未设置房间地图，请前往系统设置页面设置地图图片。
        </div>
      ) : (
        <div
          className="map-container"
          ref={containerRef}
          style={{ backgroundImage: `url(${mapImageUrl})` }}
        >
          {devices.map((device) => (
            <DeviceMarker
              key={device.id}
              device={device}
              containerRef={containerRef}
              onUpdateCoordinates={updateCoordinates}
              onOpenModal={handleOpenModal}
            />
          ))}
        </div>
      )}

      {selectedDevice && (
        <DeviceDetailModal
          device={selectedDevice}
          onClose={() => setSelectedDevice(null)}
          onSave={handleSaveDeviceModal}
        />
      )}
    </div>
  );
};

export default MapEditorPage;
