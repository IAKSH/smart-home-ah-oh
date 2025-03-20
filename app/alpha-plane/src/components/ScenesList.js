// components/ScenesList.js
import React from 'react';

const ScenesList = () => {
  // 示例场景数据
  const scenes = [
    { id: 1, name: '早晨模式', description: '自动开启灯光与窗帘' },
    { id: 2, name: '离家模式', description: '关闭所有设备并启动安防' },
    { id: 3, name: '夜间模式', description: '调暗灯光，激活安防系统' }
  ];

  return (
    <div className="scenes-list">
      {scenes.map(scene => (
        <div key={scene.id} className="scene-card">
          <h3>{scene.name}</h3>
          <p>{scene.description}</p>
        </div>
      ))}
    </div>
  );
};

export default ScenesList;
