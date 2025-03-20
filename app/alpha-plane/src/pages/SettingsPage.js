// pages/SettingsPage.js
import React, { useState, useEffect } from 'react';
import { useNavigate } from 'react-router-dom';
import { useAuth } from '../context/AuthContext';
import './SettingsPage.css';

const SettingsPage = () => {
  // 基本设置：系统名称、时区和地图图片
  const [systemName, setSystemName] = useState('');
  const [timeZone, setTimeZone] = useState('UTC+8');
  const [mapImageUrl, setMapImageUrl] = useState('');
  const [uploading, setUploading] = useState(false);

  // 账户设置：修改密码
  const [oldPassword, setOldPassword] = useState('');
  const [newPassword, setNewPassword] = useState('');
  const [confirmPassword, setConfirmPassword] = useState('');
  const [passwordMessage, setPasswordMessage] = useState('');

  const { logout } = useAuth();
  const navigate = useNavigate();

  // 组件加载时读取之前保存的设置
  useEffect(() => {
    const storedSystemName = localStorage.getItem('systemName') || '';
    const storedMapUrl = localStorage.getItem('mapImageUrl') || '';
    setSystemName(storedSystemName);
    setMapImageUrl(storedMapUrl);
    // 如果没有保存过密码，初始化为 "password"
    if (!localStorage.getItem('dummyPassword')) {
      localStorage.setItem('dummyPassword', 'password');
    }
  }, []);

  // Dummy 图片上传函数（模拟上传）
  const dummyUploadImage = (file) => {
    return new Promise((resolve, reject) => {
      setUploading(true);
      setTimeout(() => {
        const uploadedUrl = URL.createObjectURL(file);
        setUploading(false);
        resolve(uploadedUrl);
      }, 1500);
    });
  };

  // 文件上传处理
  const handleFileChange = async (e) => {
    const file = e.target.files[0];
    if (!file) return;
    try {
      const uploadedUrl = await dummyUploadImage(file);
      setMapImageUrl(uploadedUrl);
      localStorage.setItem('mapImageUrl', uploadedUrl);
    } catch (error) {
      console.error('图片上传失败:', error);
    }
  };

  // 保存基本设置
  const handleBasicSubmit = (e) => {
    e.preventDefault();
    localStorage.setItem('systemName', systemName);
    localStorage.setItem('timeZone', timeZone);
    alert('基本设置保存成功！');
  };

  // 处理密码修改
  const handlePasswordChange = (e) => {
    e.preventDefault();
    const currentPassword = localStorage.getItem('dummyPassword');
    if (oldPassword !== currentPassword) {
      setPasswordMessage('旧密码错误！');
      return;
    }
    if (newPassword !== confirmPassword) {
      setPasswordMessage('新密码与确认密码不一致！');
      return;
    }
    localStorage.setItem('dummyPassword', newPassword);
    setPasswordMessage('密码修改成功！');
    // 清空输入
    setOldPassword('');
    setNewPassword('');
    setConfirmPassword('');
  };

  // 退出登录
  const handleLogout = () => {
    logout();
    navigate('/login');
  };

  return (
    <div className="settings-page">
      <h1>系统设置</h1>
      <div className="settings-container">
        <div className="settings-section">
          <h2>基本设置</h2>
          <form onSubmit={handleBasicSubmit} className="settings-form">
            <div className="form-group">
              <label htmlFor="systemName">系统名称:</label>
              <input
                type="text"
                id="systemName"
                placeholder="请输入系统名称"
                value={systemName}
                onChange={(e) => setSystemName(e.target.value)}
              />
            </div>
            <div className="form-group">
              <label htmlFor="timeZone">时区:</label>
              <select
                id="timeZone"
                value={timeZone}
                onChange={(e) => setTimeZone(e.target.value)}
              >
                <option value="UTC+8">UTC+8</option>
                <option value="UTC+0">UTC+0</option>
              </select>
            </div>
            <div className="form-group">
              <label htmlFor="mapImage">上传地图图片:</label>
              <input
                type="file"
                id="mapImage"
                accept="image/*"
                onChange={handleFileChange}
              />
              {uploading && <p>上传中...</p>}
              {mapImageUrl && (
                <div className="map-preview">
                  <img src={mapImageUrl} alt="地图预览" />
                </div>
              )}
            </div>
            <button type="submit" disabled={uploading}>
              保存基本设置
            </button>
          </form>
        </div>

        <div className="settings-section account-section">
          <h2>账户设置</h2>
          <form onSubmit={handlePasswordChange} className="password-form">
            <div className="form-group">
              <label htmlFor="oldPassword">旧密码:</label>
              <input
                type="password"
                id="oldPassword"
                placeholder="请输入旧密码"
                value={oldPassword}
                onChange={(e) => setOldPassword(e.target.value)}
              />
            </div>
            <div className="form-group">
              <label htmlFor="newPassword">新密码:</label>
              <input
                type="password"
                id="newPassword"
                placeholder="请输入新密码"
                value={newPassword}
                onChange={(e) => setNewPassword(e.target.value)}
              />
            </div>
            <div className="form-group">
              <label htmlFor="confirmPassword">确认密码:</label>
              <input
                type="password"
                id="confirmPassword"
                placeholder="请再次输入新密码"
                value={confirmPassword}
                onChange={(e) => setConfirmPassword(e.target.value)}
              />
            </div>
            {passwordMessage && (
              <p className="password-message">{passwordMessage}</p>
            )}
            <button type="submit">修改密码</button>
          </form>
          <button className="logout-button" onClick={handleLogout}>
            退出登录
          </button>
        </div>
      </div>
    </div>
  );
};

export default SettingsPage;
