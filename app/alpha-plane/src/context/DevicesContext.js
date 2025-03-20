// context/DevicesContext.js
import React, { createContext, useContext, useState } from 'react';
import { initialDevices } from '../devicesData';

const DevicesContext = createContext();

export const DevicesProvider = ({ children }) => {
  const [devices, setDevices] = useState(initialDevices);

  const updateDevice = updatedDevice => {
    setDevices(prevDevices =>
      prevDevices.map(device =>
        device.id === updatedDevice.id ? updatedDevice : device
      )
    );
  };

  return (
    <DevicesContext.Provider value={{ devices, updateDevice }}>
      {children}
    </DevicesContext.Provider>
  );
};

export const useDevices = () => useContext(DevicesContext);
