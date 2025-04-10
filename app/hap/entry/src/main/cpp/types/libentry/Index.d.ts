export const add: (a: number, b: number) => number;
export const connectServer: (ip: string, port: string) => void;
export const fetchDevices: () => string;
export const fetchDevice: (deviceId: string) => string;
export const deleteDevice: (deviceid: string) => void;
export const initUdpContext: () => void;
export const discoverServer: () => string;
export const napiCallbackTest: (callback: Function) => void;

export const registerMQTTCallback: (callback: Function) => void;
export const subscribe: (topic: string) => void;
export const publish: (topic: string, payload: string) => void;
export const connectBroker: (broker: string, clientId: string) => void;