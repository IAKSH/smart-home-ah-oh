// pages/DevicesPage.js
import React from 'react';
import DeviceCard from '../components/DeviceCard';
import { useDevices } from '../context/DevicesContext';
import './DevicesPage.css';

const DevicesPage = () => {
  const { devices, updateDevice } = useDevices();

  return (
    <div className="card-grid">
      {devices.map(device => (
        <DeviceCard key={device.id} device={device} onUpdate={updateDevice} />
      ))}
    </div>
  );
};

export default DevicesPage;
