import { router } from '@kit.ArkUI';
import { DeviceAttribItem } from '../models/Device';
import testNapi from "libentry.so"
import { hilog } from '@kit.PerformanceAnalysisKit';
import { Device } from "../models/Device"

interface RouterParams {
  device: Device;
}

type DeviceAttrib = DeviceAttribItem[];

//
// 字段配置接口
//
interface FieldConfig {
  name: string;
  unit: string;
}

@Entry
@Component
struct DeviceDetail {
  // 通过 @State 声明设备数据，默认使用空设备对象初始化
  @State device: Device = new Device();

  // 字段配置映射，同列表页面保持一致
  private fieldConfig: Map<string, FieldConfig> = new Map([
    ['temperature', { name: '温度', unit: '℃' } as FieldConfig],
    ['humidity', { name: '湿度', unit: '%' }],
    ['power', { name: '功率', unit: 'W' }],
    ['voltage', { name: '电压', unit: 'V' }],
    ['co2', { name: '二氧化碳', unit: 'ppm' }],
    ['alert', { name: '警报', unit: '' }]
  ]);

  updateRemoteDevice(key: string, newValue: string | number | boolean): void {
    hilog.info(0, 'DEBUG', 'update attrib %{public}s to %{public}s', key, newValue);
    class Data {
      value: string = ''
    }
    let data: Data = { value: newValue.toString() };
    testNapi.publish(`/device/${this.device.id}/attrib/${key}`, JSON.stringify(data));
  }

  // 根据目标类型将输入字符串转换为对应类型值
  private convertValue(value: string, targetType: string): string | number | boolean {
    if (targetType === 'number') {
      const num = Number(value);
      return isNaN(num) ? 0 : num;
    } else if (targetType === 'boolean') {
      return value.toLowerCase() === 'true';
    }
    return value;
  }

  // 新增：提交保存修改时调用，将所有可写字段的修改统一发送后端
  private submitChanges(): void {
    for (let item of this.device.attrib) {
      if (item.w) {
        this.updateRemoteDevice(item.key, item.value);
      }
    }
  }

  // 用于根据属性是否可写来生成相应的输入或只读展示组件
  @Builder
  InputComponent(item: DeviceAttribItem) {
    if (item.w) {
      TextInput({ text: item.value.toString() })
        .fontSize(16)
        .layoutWeight(2)
        .onChange((value: string) => {
          // 仅更新本地属性值，不立即发送请求
          const newVal = this.convertValue(value, typeof item.value);
          item.value = newVal;
        });
    } else {
      Text(item.value.toString() + (this.fieldConfig.get(item.key)?.unit ?? ''))
        .fontSize(16)
        .fontWeight(FontWeight.Medium)
        .layoutWeight(2)
        .textAlign(TextAlign.End);
    }
  }

  // 属性项构建器：展示属性名称和对应的值（以及可能的输入框）
  @Builder
  AttribItem(item: DeviceAttribItem, displayName: string) {
    Column() {
      Row() {
        // 左侧名称部分，固定占位
        Text(displayName)
          .fontSize(16)
          .fontColor('#666')
          .layoutWeight(1)
          .textAlign(TextAlign.Start);
        // 右侧输入/只读组件直接调用 InputComponent
        this.InputComponent(item);
      }
      .padding(4)
      .backgroundColor('#F0F0F0')
      .borderRadius(4);
      Row() {
        // 分割线
      }
      .width("100%")
      .height(1)
      .backgroundColor("#ddd")
      .margin({ top: 4 });
    }
    .margin({ bottom: 4 });
  }

  // 用于根据 key 获取显示名称的方法（避免直接在UI中使用局部变量）
  private getDisplayName(key: string): string {
    const config = this.fieldConfig.get(key);
    return config ? config.name : key;
  }

  build() {
    Column() {
      // 滚动详情区域：包含标题、图片、基本信息和属性数据
      Scroll() {
        Column() {
          // 标题区域及设备图片
          Column() {
            Text("设备详情")
              .fontSize(20)
              .fontWeight(FontWeight.Medium)
              .textAlign(TextAlign.Center)
              .width("100%")
              .margin({ bottom: 12 });
            Image(this.device.icon)
              .width("100%")
              .height(200);
          }
          .padding({ left: 16, right: 16, top: 20, bottom: 12 })
          .backgroundColor('#FFF')
          .shadow({ radius: 4, color: '#1A000000', offsetX: 1, offsetY: 1 })
          .width("100%");

          // 设备基本信息：名称和状态
          Row() {
            Text("名称：").fontSize(14);
            Text(this.device.name).fontSize(14);
          }
          .margin({ top: 12, bottom: 8 })
          .width("100%");
          Row() {
            Text("状态：").fontSize(14);
            Text(this.device.status)
              .fontSize(14)
              .fontColor(this.device.status === '在线' ? '#07C160' : '#999');
          }
          .margin({ bottom: 16 })
          .width("100%");

          // 遍历显示所有属性数据
          ForEach(this.device.attrib, (item: DeviceAttribItem) => {
            this.AttribItem(item, this.getDisplayName(item.key));
          });
        }
        .padding({ left: 16, right: 16, top: 16, bottom: 120 })
        .backgroundColor('#FFF')
        .width("100%");
      }
      .layoutWeight(1)
      .width("100%")
      .backgroundColor('#F5F5F5');

      // 固定在页面底部的操作按钮区域（不随滚动内容移动）
      Row() {
        Button("💾 保存修改")
          .onClick(() => {
            // 点击按钮后统一提交修改
            this.submitChanges();
          })
          .layoutWeight(1)
          .backgroundColor("transparent")
          .fontColor("#007AFF")
          .borderWidth(0);
        Button("🗑 删除设备")
          .onClick(() => {
            //console.log("删除设备");
          })
          .layoutWeight(1)
          .backgroundColor("transparent")
          .fontColor("#FF3B30")
          .borderWidth(0)
          .margin({ left: 8 });
      }
      .height(60)
      .backgroundColor("#FFF")
      .width("100%");
    }
    .width("100%")
    .height("100%")
    .onAppear(() => {
      // 在页面出现时，通过路由参数更新设备信息
      const params = router.getParams() as RouterParams;
      this.device = params.device;
    });
  }
}
