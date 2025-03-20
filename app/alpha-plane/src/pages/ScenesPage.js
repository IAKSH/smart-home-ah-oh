// pages/ScenesPage.js
import React from 'react';
import ScenesList from '../components/ScenesList';
import './ScenesPage.css';

const ScenesPage = () => {
  return (
    <div className="scenes-page">
      <h1>场景控制</h1>
      <ScenesList />
    </div>
  );
};

export default ScenesPage;
