import React, { useState } from 'react';
import ReactDOM from 'react-dom';
import './DeviceCard.css';

const defaultImageUrl = 'device.jpg';

const DeviceCard = ({ device, onUpdate }) => {
  const [showModal, setShowModal] = useState(false);
  const [editedDevice, setEditedDevice] = useState(device);

  // 辅助函数：提取属性名称，将 topic 前导斜杠去掉
  const getAttribKey = (metaItem) => {
    if (metaItem.topic && metaItem.topic.startsWith('/')) {
      return metaItem.topic.substring(1);
    }
    return metaItem.topic;
  };

  // 处理属性编辑更新，仅更新 attrib 部分
  const handleFieldChange = (field, newValue) => {
    setEditedDevice((prev) => ({
      ...prev,
      attrib: {
        ...prev.attrib,
        [field]: newValue,
      },
    }));
  };

  const saveChanges = () => {
    if (onUpdate) {
      onUpdate(editedDevice);
    }
    setShowModal(false);
  };

  // 根据 meta 定义动态渲染属性编辑控件
  const renderAttribField = (metaItem) => {
    const key = getAttribKey(metaItem);
    if (metaItem.rw === 'rw') {
      return (
        <input
          type="text"
          value={editedDevice.attrib ? editedDevice.attrib[key] || '' : ''}
          onChange={(e) => handleFieldChange(key, e.target.value)}
        />
      );
    }
    return <span>{device.attrib ? String(device.attrib[key]) : ''}</span>;
  };

  // 根据设备数据判断在线状态
  let isOnline = false;
  if (device.heartbeat && device.heartbeat.status === 'online') {
    isOnline = true;
  }
  // 如果有 will 消息，则认为设备离线
  if (device.will) {
    isOnline = false;
  }

  // 定义 Modal 内容（点击背景或"×"可以关闭）
  // DeviceCard.js 部分修订
  const modalContent = (
    <div
      className="modal-overlay"
      onClick={(e) => {
        // 只有当点击的是 modal-overlay 本身时才关闭弹窗
        if (e.currentTarget === e.target) {
          setShowModal(false);
        }
      }}
    >
      <div className="modal-content" onClick={(e) => e.stopPropagation()}>
        <button className="modal-close-icon" onClick={() => setShowModal(false)}>
          &times;
        </button>
        <h2>{device.name} 详细信息</h2>
        {device.meta &&
          device.meta.attrib &&
          device.meta.attrib.map((metaItem) => {
            const key = getAttribKey(metaItem);
            return (
              <div key={key} className="modal-field">
                <strong>
                  {key} ({metaItem.desc}):
                </strong>{' '}
                {renderAttribField(metaItem)}
              </div>
            );
          })}
        {device.heartbeat && (
          <div className="modal-field">
            <strong>状态:</strong> {device.heartbeat.status || '未知'}
          </div>
        )}
        {device.will && (
          <div className="modal-field">
            <strong>离线原因:</strong> {device.will.will || '未提供'}
          </div>
        )}
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
      <div className="device-image-wrapper">
        <img
          src={device.imageUrl || defaultImageUrl}
          alt={device.name}
          className="device-image"
          onError={(e) => {
            e.target.onerror = null;
            e.target.src = defaultImageUrl;
          }}
        />
      </div>
      <div className="device-header">
        {/* 状态指示器 */}
        <span
          className={`status-indicator ${isOnline ? 'status-online' : 'status-offline'
            }`}
        />
        <h3 className="device-title">{device.name}</h3>
      </div>
      <div className="device-info">
        {device.meta && device.meta.attrib ? (
          device.meta.attrib.map((metaItem) => {
            const key = getAttribKey(metaItem);
            return (
              <p key={key}>
                <strong>{key}:</strong> {device.attrib ? String(device.attrib[key]) : ''}
              </p>
            );
          })
        ) : (
          <p>暂无设备数据</p>
        )}
        {device.heartbeat && (
          <p>
            <strong>状态:</strong> {device.heartbeat.status || '未知'}
          </p>
        )}
      </div>
      {showModal &&
        ReactDOM.createPortal(modalContent, document.getElementById('modal-root'))}
    </div>
  );
};

export default DeviceCard;
