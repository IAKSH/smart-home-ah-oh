// pages/LogsPage.js
import React, { useEffect, useState } from 'react';
import './LogsPage.css';

const LogsPage = () => {
  const [logs, setLogs] = useState([]);

  useEffect(() => {
    // 模拟日志数据更新，可替换为实际 API 调用
    const fakeLogs = [
      '2023-10-07 12:00:00: 系统启动',
      '2023-10-07 12:05:00: 设备上线：智能温控器',
      '2023-10-07 12:06:00: 设备错误：智能摄像头'
    ];
    setLogs(fakeLogs);
  }, []);

  return (
    <div className="logs-page">
      <h1>日志监控</h1>
      <ul className="logs-list">
        {logs.map((log, index) => (
          <li key={index}>{log}</li>
        ))}
      </ul>
    </div>
  );
};

export default LogsPage;
