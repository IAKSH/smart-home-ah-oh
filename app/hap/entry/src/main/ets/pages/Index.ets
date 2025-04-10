import { router } from '@kit.ArkUI';
import { BusinessError } from '@kit.BasicServicesKit';
import { Server } from '../models/Server';
import { setActiveServer } from '../data/ServerStore';
import { fetchData } from '../data/FetchData'

@Entry
@Component
struct Index {
    @State servers: Server[] = [
        {
            name: "服务器A",
            ip: "192.168.31.110",
            description: "Server on OrangePi 4 LTS for debug",
            icon: "resources/base/media/startIcon.png"
        },
        {
            name: "服务器B",
            ip: "192.168.177.131",
            description: "Server on VMWare for debug",
            icon: "resources/base/media/startIcon.png"
        },
        {
            name: "服务器C",
            ip: "192.168.1.103",
            description: "dummy server",
            icon: "resources/base/media/startIcon.png"
        }
    ];

    onPageShow() {
        setInterval(() => {
            fetchData();
        },1000);
    }

    build() {
        Column() {
            // 固定顶部标题栏
            Row() {
                Text("🔗 连接到服务器")
                    .fontSize(18)
                    .fontWeight(FontWeight.Bold)
                    .textAlign(TextAlign.Center)
                    .layoutWeight(1)
                    .fontColor("#333");
            }
            .padding({ top: 20, bottom: 20 })
            .backgroundColor("#FFFFFF");

            // 分割线
            Row() {}
            .width("100%")
            .height(1)
            .backgroundColor("#E0E0E0");

            // 服务器列表部分
            Scroll() {
                Column() {
                    ForEach(this.servers, (server: Server) => {
                        Row() {
                            Image(server.icon)
                                .width(50)
                                .height(50)
                                .borderRadius(25)
                                .margin({ right: 12 });
                            Column() {
                                Text("🖥️ " + server.name)
                                    .fontSize(20)
                                    .fontWeight(FontWeight.Medium)
                                    .textAlign(TextAlign.Start)
                                    .fontColor("#000");
                                Text(server.ip)
                                    .fontSize(16)
                                    .textAlign(TextAlign.Start)
                                    .fontColor("#555");
                                Text("💡 " + server.description)
                                    .fontSize(14)
                                    .textAlign(TextAlign.Start)
                                    .fontColor("#888888");
                            }
                            .layoutWeight(1);
                        }
                        .padding({ top: 12, bottom: 12, left: 12, right: 12 })
                        .backgroundColor("#FFFFFF")
                        .borderRadius(8)
                        .clip(true)
                        .shadow({ radius: 4, color: "#00000022", offsetX: 0, offsetY: 2 })
                        .onClick(() => {
                            // 先将选中的服务器保存到全局状态
                            setActiveServer(server);
                            router.pushUrl({ url: "pages/TestPage" })
                                .then(() => {
                                    console.info("Succeeded in jumping to DeviceDashboard.");
                                })
                                .catch((err: BusinessError) => {
                                    console.error("Failed to jump. Error: " + err.message);
                                });
                        })
                        .margin({ bottom: 16 });
                    });
                }
                .width("100%")
                .padding({ left: 20, right: 20 });
            }
            .layoutWeight(1);
        }
        .width("100%")
        .height("100%")
        .backgroundColor("#F0F0F0");
    }
}
