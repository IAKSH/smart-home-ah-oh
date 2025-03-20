// pages/LoginPage.js
import React, { useState } from 'react';
import { useNavigate, Navigate } from 'react-router-dom';
import { useAuth } from '../context/AuthContext';
import './LoginPage.css';

const LoginPage = () => {
  const [username, setUsername] = useState('');
  const [password, setPassword] = useState('');
  const [error, setError] = useState('');
  const { authToken, login } = useAuth();
  const navigate = useNavigate();

  // 如果已登录，则自动跳转到 /devices
  if (authToken) {
    return <Navigate to="/devices" replace />;
  }

  const handleLogin = (e) => {
    e.preventDefault();
    // Dummy 验证：只有用户名为 'admin' 且密码为 'password' 才登录成功
    if (username === 'admin' && password === 'password') {
      login('dummy-token');
      navigate('/devices');
    } else {
      setError('用户名或密码错误！');
    }
  };

  return (
    <div className="login-page">
      <h1>系统登录</h1>
      <form onSubmit={handleLogin} className="login-form">
        {error && <p className="error">{error}</p>}
        <div className="form-group">
          <label htmlFor="username">用户名:</label>
          <input
            type="text"
            id="username"
            placeholder="请输入用户名"
            value={username}
            onChange={(e) => setUsername(e.target.value)}
          />
        </div>
        <div className="form-group">
          <label htmlFor="password">密码:</label>
          <input
            type="password"
            id="password"
            placeholder="请输入密码"
            value={password}
            onChange={(e) => setPassword(e.target.value)}
          />
        </div>
        <button type="submit">登录</button>
      </form>
    </div>
  );
};

export default LoginPage;
