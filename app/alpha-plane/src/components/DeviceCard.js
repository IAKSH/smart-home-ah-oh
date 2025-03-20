import React, { useState } from 'react';
import ReactDOM from 'react-dom';
import './DeviceCard.css';

// 默认设备图片 URL（请替换为实际图片路径或 URL）
const defaultImageUrl = 'logo512.png';

// 设备卡片组件
const DeviceCard = ({ device, onUpdate }) => {
  // 控制详情弹窗的显隐状态
  const [showModal, setShowModal] = useState(false);
  // 保存设备数据的局部状态（便于编辑）
  const [editedDevice, setEditedDevice] = useState(device);

  // 当需要修改值时更新对应字段
  const handleFieldChange = (field, newValue) => {
    setEditedDevice((prev) => ({
      ...prev,
      [field]: {
        ...prev[field],
        value: newValue
      }
    }));
  };

  // 点击“保存修改”，调用回调，并关闭弹窗
  const saveChanges = () => {
    if (onUpdate) {
      onUpdate(editedDevice);
    }
    setShowModal(false);
  };

  // 若文本长度超过 maxLength，则进行截断显示
  const truncate = (text, maxLength = 50) => {
    if (text.length > maxLength) {
      return text.slice(0, maxLength) + '...';
    }
    return text;
  };

  // 定义Modal内容，使用Portal渲染到独立的DOM节点
  const modalContent = (
    <div
      className="modal-overlay"
      onClick={(e) => {
        e.stopPropagation(); // 阻止事件冒泡到父级
        if (e.target.className === 'modal-overlay') {
          setShowModal(false);
        }
      }}
    >
      <div
        className="modal-content"
        onClick={(e) => e.stopPropagation()} // 确保点击内容区不关闭弹窗
      >
        <h2>{device.name} 详细信息</h2>
        {Object.keys(editedDevice).map((key) => {
          if (['id', 'name', 'imageUrl'].includes(key)) {
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
          <button className="modal-save" onClick={saveChanges}>
            保存修改
          </button>
          <button className="modal-close" onClick={() => setShowModal(false)}>
            关闭
          </button>
        </div>
      </div>
    </div>
  );

  return (
    <div className="device-card" onClick={() => setShowModal(true)}>
      <img
        src={device.imageUrl || defaultImageUrl}
        alt={device.name}
        className="device-image"
        onError={(e) => {
          e.target.onerror = null; // 防止无限循环（若默认图片也出错）
          e.target.src = defaultImageUrl;
        }}
      />
      <h3 className="device-title">{device.name}</h3>
      <div className="device-info">
        {Object.keys(device).map((key) => {
          if (['id', 'name', 'imageUrl'].includes(key)) {
            return null;
          }
          const field = device[key];
          if (field && typeof field === 'object' && field.hasOwnProperty('value')) {
            return (
              <p key={key}>
                <strong>{key}:</strong> {truncate(String(field.value))}
              </p>
            );
          } else {
            return (
              <p key={key}>
                <strong>{key}:</strong> {String(device[key])}
              </p>
            );
          }
        })}
      </div>
      {/* 使用React Portal来渲染独立的弹窗 */}
      {showModal &&
        ReactDOM.createPortal(
          modalContent,
          document.getElementById('modal-root')
        )}
    </div>
  );
};

export default DeviceCard;
