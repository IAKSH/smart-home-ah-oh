// devicesData.js

// 初始设备数据
export const initialDevices = [
    {
      id: 'device1',
      name: '智能温控器',
      imageUrl: 'https://via.placeholder.com/150',
      temperature: { value: '22℃', rw: 'rw' },
      humidity: { value: '40%', rw: '' },
      location: { value: '客厅', rw: 'rw' },
      description: {
        value: '这是一台支持远程控制且节能的智能温控器，详细描述信息非常冗长，需要在卡片内进行隐藏显示处理。',
        rw: ''
      }
    },
    {
      id: 'device2',
      name: '智能摄像头',
      imageUrl: 'https://via.placeholder.com/150',
      resolution: { value: '1080p', rw: '' },
      status: { value: 'active', rw: '' },
      description: {
        value: '安装在门前，用于安保监控的高清摄像头，提供全天候视频监控。',
        rw: ''
      }
    }
  ];
  
  // 更新设备列表的简单函数（视具体业务可扩展）
  export const updateDeviceInList = (devices, updatedDevice) => {
    return devices.map(device =>
      device.id === updatedDevice.id ? updatedDevice : device
    );
  };
  