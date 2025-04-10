@Entry
@Component
export default struct SettingsCenter {
  // 状态变量：用于存储各项数据
  @State radioSelection: number = 0;              // 单选：取值 0～2
  @State multiSelected: boolean[] = [false, false, false]; // 多选（模拟复选框）3项
  @State singleLineInput: string = "";            // 单行文本输入内容
  @State multiLineInput: string = "";             // 多行文本输入内容
  @State switchValue: boolean = false;            // 开关状态

  // Helper：返回单选项文本（选中用“●”，未选用“○”）
  radioIndicator(index: number): string {
    return this.radioSelection == index ? "●" : "○";
  }

  // Helper：切换多选项状态
  toggleMultiSelection(index: number) {
    if (this.multiSelected[index]) {
      this.multiSelected[index] = false;
    } else {
      this.multiSelected[index] = true;
    }
  }

  @Builder
  build() {
    Scroll() {
      Column() {
        //--------------------------
        // 顶部版本信息区域
        //--------------------------
        Column() {
          Text("📱 本APP版本：v1.0.0")
            .fontSize(16)
            .fontWeight(FontWeight.Medium)
            .textAlign(TextAlign.Center)
            .fontColor("#333");
        }
        .padding({ top: 10, bottom: 10 })
        .margin({ bottom: 20 });

        //--------------------------
        // (1) 按钮控件示例
        //--------------------------
        Row() {
          Text("🚀 按钮")
            .fontSize(14)
            .layoutWeight(1)
            .textAlign(TextAlign.Start);
          Button("点击执行")
            .onClick(() => { console.log("按钮被点击！"); })
            .backgroundColor("transparent")
            .borderWidth(0)
            .fontColor("#007AFF")
            .layoutWeight(1)
            .align(Alignment.End);
        }
        .padding({ top: 8, bottom: 8, left: 12, right: 12 })
        //.border({ bottom: { width: 1, color: "#E0E0E0" }});

        //--------------------------
        // (2) 单选控件示例 —— 使用无边框按钮模拟
        //--------------------------
        Column() {
          Text("📻 单选框")
            .fontSize(14)
            .textAlign(TextAlign.Start)
            .margin({ bottom: 4 });
          Row() {
            Button(this.radioIndicator(0) + " 选项A")
              .onClick(() => { this.radioSelection = 0; })
              .backgroundColor("transparent")
              .borderWidth(0)
              .fontColor(this.radioSelection == 0 ? "#007AFF" : "#888888")
              .layoutWeight(1)
              .align(Alignment.Center);
            Button(this.radioIndicator(1) + " 选项B")
              .onClick(() => { this.radioSelection = 1; })
              .backgroundColor("transparent")
              .borderWidth(0)
              .fontColor(this.radioSelection == 1 ? "#007AFF" : "#888888")
              .layoutWeight(1)
              .align(Alignment.Center)
              .margin({ left: 8 });
            Button(this.radioIndicator(2) + " 选项C")
              .onClick(() => { this.radioSelection = 2; })
              .backgroundColor("transparent")
              .borderWidth(0)
              .fontColor(this.radioSelection == 2 ? "#007AFF" : "#888888")
              .layoutWeight(1)
              .align(Alignment.Center)
              .margin({ left: 8 });
          }
        }
        .padding({ top: 8, bottom: 8, left: 12, right: 12 })
        //.border({ bottom: { width: 1, color: "#E0E0E0" }})
        .margin({ bottom: 10 });

        //--------------------------
        // (3) 多选控件示例 —— 模拟复选框（使用无边框按钮）
        //--------------------------
        Column() {
          Text("☑ 多选框")
            .fontSize(14)
            .textAlign(TextAlign.Start)
            .margin({ bottom: 4 });
          Row() {
            Button("选项1 " + (this.multiSelected[0] ? "☑" : "☐"))
              .onClick(() => { this.toggleMultiSelection(0); })
              .backgroundColor("transparent")
              .borderWidth(0)
              .fontColor(this.multiSelected[0] ? "#007AFF" : "#888888")
              .layoutWeight(1)
              .align(Alignment.Center);
            Button("选项2 " + (this.multiSelected[1] ? "☑" : "☐"))
              .onClick(() => { this.toggleMultiSelection(1); })
              .backgroundColor("transparent")
              .borderWidth(0)
              .fontColor(this.multiSelected[1] ? "#007AFF" : "#888888")
              .layoutWeight(1)
              .align(Alignment.Center)
              .margin({ left: 8 });
            Button("选项3 " + (this.multiSelected[2] ? "☑" : "☐"))
              .onClick(() => { this.toggleMultiSelection(2); })
              .backgroundColor("transparent")
              .borderWidth(0)
              .fontColor(this.multiSelected[2] ? "#007AFF" : "#888888")
              .layoutWeight(1)
              .align(Alignment.Center)
              .margin({ left: 8 });
          }
        }
        .padding({ top: 8, bottom: 8, left: 12, right: 12 })
        //.border({ bottom: { width: 1, color: "#E0E0E0" }})
        .margin({ bottom: 10 });

        //--------------------------
        // (4) 单行文本输入 —— 使用官方 TextField API
        //--------------------------
        Row() {
          Text("💬 单行输入")
            .fontSize(14)
            .layoutWeight(0.3)
            .textAlign(TextAlign.Start);
          TextInput({
            placeholder: "请输入单行文本",
          })
            .onChange((value: string) => { this.singleLineInput = value; })
            .layoutWeight(0.7)
            .padding(8)
            .backgroundColor("#FFFFFF")
            .borderWidth(0)
            .textAlign(TextAlign.Start);
        }
        .padding({ top: 8, bottom: 8, left: 12, right: 12 })
        //.border({ bottom: { width: 1, color: "#E0E0E0" }})
        .margin({ bottom: 10 });

        //--------------------------
        // (5) 多行文本输入 —— 使用官方 TextField API（multiline 模式）
        //--------------------------
        Column() {
          Text("📝 多行输入")
            .fontSize(14)
            .textAlign(TextAlign.Start)
            .padding({ left: 12, right: 12, bottom: 4 });
          TextArea({
            placeholder: "请输入多行文本"
          })
            .onChange((value: string) => { this.multiLineInput = value; })
            .height(80)
            .padding({ top: 8, bottom: 8, left: 12, right: 12 })
            .backgroundColor("#FFFFFF")
            .borderWidth(0)
            .textAlign(TextAlign.Start);
          Row() {}
          .width("100%")
          .height(1)
          .backgroundColor("#E0E0E0");
        }
        .margin({ bottom: 10 });

        //--------------------------
        // (6) 开关控件 —— 使用官方 Switch API
        //--------------------------
        //Row() {
        //  Text("⚙️ 开关")
        //    .fontSize(14)
        //    .layoutWeight(1)
        //    .textAlign(TextAlign.Start);
        //  Switch(this.switchValue)
        //    .onChange((value: boolean) => { this.switchValue = value; })
        //    .layoutWeight(1)
        //    .textAlign(TextAlign.End);
        //}
        //.padding({ top: 8, bottom: 8, left: 12, right: 12 })
        //.border({ bottom: { width: 1, color: "#E0E0E0" }})
        //.margin({ bottom: 20 });

        //--------------------------
        // (7) 断开连接区域 —— 放置在页面最后，使用红色文字
        //--------------------------
        Column() {
          Button("🔴 断开连接")
            .onClick(() => { console.log("断开与智能家居主控服务器的连接"); })
            .padding({ top: 12, bottom: 12, left: 20, right: 20 })
            .backgroundColor("transparent")
            .borderWidth(0)
            .fontColor("#FF0000");
        }
        .margin({ top: 20 });
      }
      .width("100%");
    }
    .width("100%")
      .height("100%");
  }
}
