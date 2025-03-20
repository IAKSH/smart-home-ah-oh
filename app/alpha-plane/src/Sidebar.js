// Sidebar.js
import React, { useState, useEffect } from 'react';
import { NavLink } from 'react-router-dom';
import './Sidebar.css';

const Sidebar = () => {
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

  // 定义路由和标签，同时添加图标（此处用 emoji 作为示例）
  const routes = [
    { path: '/devices', label: '设备管理', icon: '🔌' },
    { path: '/scenes', label: '场景控制', icon: '🎭' },
    { path: '/map-editor', label: '地图编辑器', icon: '🗺️' },
    { path: '/settings', label: '系统设置', icon: '⚙️' },
    { path: '/logs', label: '日志监控', icon: '📜' },
  ];

  return (
    <div className="sidebar">
      <div className="project-name">{systemName}</div>
      <ul className="nav">
        {routes.map((route) => (
          <li key={route.path}>
            <NavLink
              to={route.path}
              className={({ isActive }) =>
                isActive ? 'nav-item active' : 'nav-item'
              }
            >
              <span className="icon">{route.icon}</span>
              <span className="label">{route.label}</span>
            </NavLink>
          </li>
        ))}
      </ul>
    </div>
  );
};

export default Sidebar;
