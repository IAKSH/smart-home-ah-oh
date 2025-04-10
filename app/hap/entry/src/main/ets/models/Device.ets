// models/Device.ets

// 定义属性项接口
export interface DeviceAttribItem {
  key: string;
  value: string | number | boolean;
  r: boolean,
  w: boolean
}

// 设备属性类型定义
export type DeviceAttrib = DeviceAttribItem[];

// 修改 Device 数据结构中的 attrib 类型
export class Device {
  id: string = '';
  name: string = '';
  status: '在线' | '离线' = '离线';
  icon: string = '';
  attrib: DeviceAttrib = [];
}

