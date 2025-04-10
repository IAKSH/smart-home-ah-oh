import SettingsCenter from './SettingsCenter';
import MyHome from './MyHome';
import DevicePanel from "./DevicePanel";
import { activeServer } from '../data/ServerStore';

@Entry
@Component
struct DeviceDashboard {
  @State selectedTab: number = 0;

  build() {
    Column() {
      // 标题栏显示当前选中的服务器信息，从全局变量 activeServer 读取
      Row() {
        Text("🖥 " + activeServer.name + " (" + activeServer.ip + ")")
          .fontSize(18)
          .fontWeight(FontWeight.Medium)
          .layoutWeight(1)
          .textAlign(TextAlign.Center);
      }
      .padding({ top: 20, bottom: 20 })
      .backgroundColor("#FFF")
      .width("100%");

      // 分割线
      Row() {}
      .width("100%")
      .height(1)
      .backgroundColor("#E0E0E0");

      // 内容区域，根据 selectedTab 显示不同页面
      Stack() {
        if (this.selectedTab == 0)
          DevicePanel();
        else if (this.selectedTab == 1)
          MyHome();
        else if (this.selectedTab == 2)
          SettingsCenter();
      }
      .layoutWeight(1)
      .width("100%");

      // 底部页签栏
      Row() {
        Button("📱 设备面板")
          .onClick(() => { this.selectedTab = 0; })
          .layoutWeight(1)
          .backgroundColor("transparent")
          .fontColor(this.selectedTab == 0 ? "#000000" : "#888888")
          .borderWidth(0);
        Button("🏠 我的家庭")
          .onClick(() => { this.selectedTab = 1; })
          .layoutWeight(1)
          .backgroundColor("transparent")
          .fontColor(this.selectedTab == 1 ? "#000000" : "#888888")
          .borderWidth(0)
          .margin({ left: 8 });
        Button("⚙️ 设置中心")
          .onClick(() => { this.selectedTab = 2; })
          .layoutWeight(1)
          .backgroundColor("transparent")
          .fontColor(this.selectedTab == 2 ? "#000000" : "#888888")
          .borderWidth(0)
          .margin({ left: 8 });
      }
      .height(60)
      .backgroundColor("#FFF")
      .position({ bottom: 0, left: 0, right: 0 });
    }
    .width("100%")
      .height("100%")
      .backgroundColor("#F5F5F5");
  }
}
