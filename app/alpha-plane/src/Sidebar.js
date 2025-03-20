// Sidebar.js
import React, { useState, useEffect } from 'react';
import { useNavigate } from 'react-router-dom';
import './Sidebar.css';

const Sidebar = () => {
  const navigate = useNavigate();

  // 从 localStorage 中获取系统名称，若不存在则默认显示 "Alpha-Plane"
  const [systemName, setSystemName] = useState(
    localStorage.getItem('systemName') || 'Alpha-Plane'
  );

  // 监听 localStorage 变化（仅在其他标签页更新时有效）
  useEffect(() => {
    const handleStorageChange = (event) => {
      if (event.key === 'systemName') {
        setSystemName(event.newValue || 'Alpha-Plane');
      }
    };
    window.addEventListener('storage', handleStorageChange);
    return () => window.removeEventListener('storage', handleStorageChange);
  }, []);

  return (
    <div className="sidebar">
      <div className="project-name">{systemName}</div>
      <ul className="nav">
        <li className="nav-item" onClick={() => navigate('/devices')}>
          设备管理
        </li>
        <li className="nav-item" onClick={() => navigate('/scenes')}>
          场景控制
        </li>
        <li className="nav-item" onClick={() => navigate('/map-editor')}>
          地图编辑器
        </li>
        <li className="nav-item" onClick={() => navigate('/settings')}>
          系统设置
        </li>
        <li className="nav-item" onClick={() => navigate('/logs')}>
          日志监控
        </li>
      </ul>
    </div>
  );
};

export default Sidebar;
