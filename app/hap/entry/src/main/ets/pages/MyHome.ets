// MyHome.ets

@Entry
@Component
export default struct MyHome {
  @Builder
  build() {
    Column() {
      Text("🏠 我的家庭内容")
        .fontSize(16)
        .textAlign(TextAlign.Center);
    }
    .width("100%")
      .height("100%")
      .justifyContent(FlexAlign.Center)
      .alignItems(HorizontalAlign.Center);
  }
}
