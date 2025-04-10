import { router } from '@kit.ArkUI';
import { Device, DeviceAttrib, DeviceAttribItem } from '../models/Device';
import testNapi from "libentry.so"
import { hilog } from '@kit.PerformanceAnalysisKit';
import { activeServer } from '../data/ServerStore';

//
// 定义字段配置接口
//
interface FieldConfig {
  name: string;
  unit: string;
}

// 定义 JSON 数据对应的接口
interface MetaAttrib {
  desc: string;
  rw: string;
  topic: string;
  type: string;
}

interface MetaData {
  attrib: MetaAttrib[];
  attrib_schema: string;
  desc: string;
  heartbeat_interval: number;
  type: string[];
}

interface DeviceMetaResponse {
  device_id: string;
  meta: MetaData;
}

interface FetchDevicesResponse {
  devices: DeviceMetaResponse[];
}

@Entry
@Component
export default struct DevicePanel {
  // 设备列表数据，主页面可通过调用前对该属性赋值来传入数据
  @State devices: Device[] = [];
  // 欢迎区域中使用的用户名，默认值可覆盖
  @State username: string = "OHOS_User";

  // 字段配置映射，与详情页保持一致
  private fieldConfig: Map<string, FieldConfig> = new Map([
    ['temperature', { name: '温度', unit: '℃' } as FieldConfig],
    ['humidity', { name: '湿度', unit: '%' }],
    ['power', { name: '功率', unit: 'W' }],
    ['voltage', { name: '电压', unit: 'V' }],
    ['co2', { name: '二氧化碳', unit: 'ppm' }],
    ['alert', { name: '警报', unit: '' }]
  ]);

  timerId: number | undefined;

  async fetchDevices(): Promise<void> {
    try {
      // 获取接口返回的 HTTP 响应字符串
      const response: string = await testNapi.fetchDevices();

      // 定位 JSON 部分，并截取出 JSON 字符串
      const jsonStart: number = response.indexOf('{');
      if (jsonStart === -1) {
        throw new Error("未找到 JSON 数据");
      }
      const jsonStr: string = response.substring(jsonStart).trim();

      // 解析 JSON 数据，假设返回数据类型为 FetchDevicesResponse
      const responseData: FetchDevicesResponse = JSON.parse(jsonStr) as FetchDevicesResponse;

      // 构造从服务器返回的新设备数组
      const devicesNew: Device[] = [];
      const deviceList: DeviceMetaResponse[] = responseData.devices;
      for (let i = 0; i < deviceList.length; i++) {
        const devItem: DeviceMetaResponse = deviceList[i];
        // 假设设备 ID 类型统一为字符串（注意：如果 Device.id 为 number，可转换：Number(devItem.device_id)）
        const devId: string = devItem.device_id;
        const devMeta: MetaData = devItem.meta;

        // 构造新设备对象
        const newDevice: Device = new Device();
        newDevice.id = devId;  // 建议保证类型一致
        newDevice.name = devMeta.desc || ("Device " + (i + 1));
        newDevice.status = "在线"; // 默认状态
        newDevice.icon = "resources/base/media/startIcon.png";
        newDevice.attrib = [];

        // 根据元数据中的 attrib 构造设备属性数组: { key, value }
        if (devMeta.attrib && Array.isArray(devMeta.attrib)) {
          for (const attr of devMeta.attrib) {
            let key: string;
            // 去掉 topic 前面的斜杠
            if (attr.topic && attr.topic.startsWith("/")) {
              key = attr.topic.substring(1);
            } else {
              key = attr.topic || "unknown";
            }
            let defaultValue: number | boolean | string;
            if (attr.type === "float" || attr.type === "int") {
              defaultValue = 0;
            } else if (attr.type === "bool") {
              defaultValue = false;
            } else {
              defaultValue = "";
            }
            // 注意：这里采用对象形式，确保合并时可以直接访问 key 和 value
            newDevice.attrib.push({ key: key, value: defaultValue, r: attr.rw[0] === 'r', w: attr.rw[0] === 'w' || attr.rw[1] === 'w' });
          }
        }
        devicesNew.push(newDevice);
      }

      // 合并数据：对比从服务器新构造的数据（devicesNew）与当前 this.devices，
      // 保留已更新（非默认）的 attrib 值，其他情况采用服务器数据
      const mergedDevices: Device[] = devicesNew.map(newDev => {
        // 在旧数据中查找同一个设备（根据ID匹配）
        const oldDev: Device | undefined = this.devices.find(d => d.id === newDev.id);
        if (oldDev) {
          // 遍历新设备的 attrib 数组，按 key 进行匹配合并
          newDev.attrib = newDev.attrib.map(newAttr => {
            const oldAttr = oldDev.attrib.find(attr => attr.key === newAttr.key);
            if (oldAttr !== undefined) {
              // 如果属性值被更新（即旧值不再是默认值），则保留旧值
              if (typeof newAttr.value === "number") {
                if (oldAttr.value !== 0) {
                  return { key: newAttr.key, value: oldAttr.value, r: newAttr.r, w: newAttr.w };
                }
              } else if (typeof newAttr.value === "boolean") {
                if (oldAttr.value !== false) {
                  return { key: newAttr.key, value: oldAttr.value, r: newAttr.r, w: newAttr.w };
                }
              } else if (typeof newAttr.value === "string") {
                if (oldAttr.value !== "") {
                  return { key: newAttr.key, value: oldAttr.value, r: newAttr.r, w: newAttr.w };
                }
              }
            }
            // 无更新则直接使用服务器返回的属性项
            return newAttr;
          });
          // 如有需要，可以保留旧设备的其它字段（例如名称、状态、图标等）
          newDev.name = oldDev.name;
          newDev.status = oldDev.status;
          newDev.icon = oldDev.icon;
        }
        return newDev;
      });

      // 更新 @State devices 变量，触发界面重渲染
      hilog.info(0, 'TestPage', 'Merged devices = %{public}s', JSON.stringify(mergedDevices));
      this.devices = mergedDevices;
    } catch (error) {
      hilog.error(0, 'TestPage', 'fetchDevices error: %{public}s', error.message);
    }
  }

  aboutToAppear() {
    testNapi.connectServer(activeServer.ip, "80");
    testNapi.connectBroker("tcp://192.168.31.110:1883", "ArkUI_MQTT_Client");

    testNapi.registerMQTTCallback((topic: string, payload: string) => {
      hilog.info(0, 'TestPage', 'received topic: %{public}s, payload: %{public}s', topic, payload);

      // 假设实际 topic 格式，例如：/device/12345/attrib/temperature
      let regex = /\/device\/([^\/]+)\/attrib\/([^\/]+)/;
      let match = topic.match(regex);
      if (match) {
        let deviceId = match[1];
        let attribId = match[2];
        hilog.info(0, 'TestPage', `device_id: ${deviceId}`);
        hilog.info(0, 'TestPage', `attrib_id: ${attribId}`);

        let matchedDevice: Device | undefined = this.devices.find((device: Device) => device.id === deviceId);
        if (matchedDevice) {
          let updated = false;
          let newAttrib: DeviceAttrib = []; // 用于构造新的 attrib 数组

          for (let j = 0; j < matchedDevice.attrib.length; j++) {
            // 直接访问对象属性，不使用数组索引方式获取字段
            let item: DeviceAttribItem = matchedDevice.attrib[j];
            if (item.key === attribId) {
              try {
                // 解析 JSON 字符串，例如：'{"value": "true"}' 或 '{"value": "3.14"}'
                class Payload {
                  value: string = '';
                }
                let parsedPayload: Payload= JSON.parse(payload);
                let rawValue = parsedPayload.value;
                let convertedValue: string | number | boolean;
                if (typeof rawValue === "string") {
                  if (rawValue.toLowerCase() === "true") {
                    convertedValue = true;
                  } else if (rawValue.toLowerCase() === "false") {
                    convertedValue = false;
                  } else if (!isNaN(Number(rawValue))) {
                    convertedValue = Number(rawValue);
                  } else {
                    convertedValue = rawValue;
                  }
                } else {
                  convertedValue = rawValue;
                }
                // 使用新的值构造新的属性项
                newAttrib.push({ key: item.key, value: convertedValue, r: item.r, w: item.w });
              } catch (error) {
                hilog.info(0, 'TestPage', "JSON解析错误: %{public}s", String(error));
                // 出现解析错误时，也将原始项复制进去，保证数组长度不变
                newAttrib.push({ key: item.key, value: item.value, r: item.r, w: item.w });
              }
              updated = true;
            } else {
              // 未匹配时，直接复制一份新的对象，确保不可变更新
              newAttrib.push({ key: item.key, value: item.value, r: item.r, w: item.w });
            }
          }

          if (!updated) {
            hilog.info(0, 'TestPage', 'unable to find attrib %{public}s in device %{public}s, ignored', attribId, deviceId);
          } else {
            // 创建新的 Device 实例，并手动复制属性
            let newDevice: Device = new Device();
            newDevice.id = matchedDevice.id;
            newDevice.name = matchedDevice.name;
            newDevice.status = matchedDevice.status;
            newDevice.icon = matchedDevice.icon;
            newDevice.attrib = newAttrib;

            // 更新 devices 数组（采用不可变更新策略）
            this.devices = this.devices.map((device: Device) => {
              if (device.id === newDevice.id) {
                return newDevice;
              } else {
                return device;
              }
            });

            hilog.info(0, 'TestPage', "[DEBUG] 更新 attrib 成功");
          }
        } else {
          hilog.info(0, 'TestPage', 'unable to find device with id: %{public}s', deviceId);
        }
      } else {
        hilog.info(0, 'TestPage', "未找到匹配项");
      }
    });

    testNapi.subscribe("/device/#");
    this.timerId = setInterval(() => {
      this.fetchDevices();
      //testNapi.napiCallbackTest((str: string) => {
      // hilog.info(0, 'TestPage', 'called callback, str = %{public}s',str);
      // testNapi.publish("/hello","123456");
      //});
    },1000);
  }

  aboutToDisappear() {
    if(this.timerId !== undefined) {
      clearInterval(this.timerId);
    }
  }

  // 属性项构建器：每项数据以名称左对齐、数值右对齐，并在下方添加分割线
  @Builder
  AttribItem(displayName: string, valueWithUnit: string) {
    Column() {
      Row() {
        Text(displayName)
          .fontSize(16)
          .fontColor('#666')
          .layoutWeight(1)
          .textAlign(TextAlign.Start);
        Text(valueWithUnit)
          .fontSize(16)
          .fontWeight(FontWeight.Medium)
          .layoutWeight(1)
          .textAlign(TextAlign.End);
      }
      .padding(4)
      .backgroundColor('#F0F0F0')
      .borderRadius(4);
      // 分割线
      Row() {}
      .width("100%")
      .height(1)
      .backgroundColor("#ddd")
      .margin({ top: 4 });
    }
    .margin({ bottom: 4 });
  }

  // 设备卡片构建器：点击卡片后导航到设备详情页
  @Builder
  DeviceCard(device: Device) {
    Column() {
      // 卡片头部：图标、名称、状态
      Row() {
        Image(device.icon)
          .width(20)
          .height(20)
          .margin({ right: 6 });
        Text(device.name)
          .fontSize(14)
          .fontWeight(FontWeight.Medium)
          .layoutWeight(1);
        Text(device.status)
          .fontSize(12)
          .fontColor(device.status === '在线' ? '#07C160' : '#999');
      }
      .width('100%')
      .padding({ bottom: 8 });
      // 设备属性数据区域：只显示前三项，多余的显示“[更多]”
      Column() {
        ForEach(
          device.attrib.slice(0, device.attrib.length > 2 ? 2 : device.attrib.length),
          (item: DeviceAttribItem) => {
            this.AttribItem(
              this.fieldConfig.get(item.key)?.name ?? item.key,
              `${item.value.toString()}${this.fieldConfig.get(item.key)?.unit ?? ''}`
            );
          }
        );
        if (device.attrib.length > 2) {
          Row() {
            Text("[更多]")
              .fontSize(14)
              .fontColor('#999');
          }
        }
      }
    }
    .padding(12)
    .backgroundColor('#FFF')
    .borderRadius(12)
    .shadow({ radius: 6, color: '#1A000000', offsetX: 1, offsetY: 1 })
    .width('46%')
    .aspectRatio(1)
    .margin({ right: 12, bottom: 12 })
    .onClick(() => {
      router.pushUrl({
        url: 'pages/DeviceDetail',
        params: { device: device }
      });
    });
  }

  // 入口 build 方法：构建整个设备列表页面（包含欢迎卡片及设备卡片矩阵）
  @Builder
  build() {
    Scroll() {
      Column() {
        // 欢迎卡片区域
        Column() {
          // 欢迎问候
          Text("😊 你好，" + this.username + "!")
            .fontSize(18)
            .fontWeight(FontWeight.Medium)
            .fontColor("#333")
            .width("100%")
            .margin({ bottom: 8 });
          // 设备统计信息
          Text("当前设备数：" + this.devices.length + "，在线数：" +
          this.devices.filter((d: Device) => d.status === '在线').length)
            .fontSize(16)
            .fontColor("#666")
            .width("100%")
            .margin({ bottom: 8 });
          // 欢迎说明文本
          Text("欢迎使用我们的智能家庭平台，让生活更智能！")
            .fontSize(14)
            .fontColor("#999")
            .width("100%")
            .margin({ bottom: 12 });
          // 添加设备按钮，右对齐
          Row() {
            Button() {
              Row() {
                Text("➕ 添加设备")
                  .fontSize(16)
                  .fontColor("#007AFF");
              }
            }
            .padding({ top: 12, bottom: 12, left: 16, right: 16 })
            .backgroundColor("transparent")
            .borderWidth(0)
            .onClick(() => {
              console.log("添加设备");
            });
          }
          .justifyContent(FlexAlign.End);
        }
        .padding({ left: 12, right: 12, top: 40, bottom: 40 })
        .backgroundColor("#FFF")
        .borderRadius(12)
        .shadow({ radius: 6, color: "#1A000000", offsetX: 1, offsetY: 1 })
        .margin({ left: 12, right: 12, bottom: 16 });

        // 设备卡片矩阵：以 Flex 布局两列显示
        Flex({ wrap: FlexWrap.Wrap, justifyContent: FlexAlign.SpaceBetween }) {
          ForEach(this.devices, (device: Device) => {
            this.DeviceCard(device);
          });
        }
        .padding(12);

        // 增加额外的底部空间，确保最后一排设备卡片不会被遮挡
        Row() {}.height(80);
      }
      .width("100%")
      // 为保证顶部内容完整展示，增加额外的上下内边距
      .padding({ top: 20, bottom: 20 });
    }
    .layoutWeight(1)
      .width("100%");
  }
}
