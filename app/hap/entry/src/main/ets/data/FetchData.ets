import http from '@ohos.net.http';

// 定义异步函数获取数据
export async function fetchData() {
    let httpRequest = http.createHttp();
    httpRequest.request("https://192.168.31.110/api/devices", (error,data) => {
            if(error) {
                console.log("error code: " + error.code + ", msg: " + error.message);
            }
            else {
                let code = data.responseCode;
                if(http.ResponseCode.OK == code) {
                    // todo
                    console.log(JSON.stringify(data.result));
                }
            }
        }
    )
}
