Dự án Smart Socket

Đồ án 2

Thành viên: 
- Phạm Nhật Tân
- Trần Văn Bắc

Các tính năng:

- Đo giá trị U, I, P từ ADE7753 tần số 10kHz.
---> Yêu cầu bật cmt vTaskDelay(pdMS_TO_TICKS(100)); dòng 236 và uncmt // esp_rom_delay_us(100); dòng 234

- Điều khiển từ xa qua Web ThingsBoard Cloud và App ThingsBoard Cloud

- SmartConfig WiFi -> Có thể config WiFi bằng điện thoại
Yêu cầu: App EspTouch. 
Bước 1: Tải app EspTouch -> Chọn EspTouch
Bước 2: Kết nối với mạng WiFi muốn thêm,
Bước 3: Nhập mật khẩu mạng.
Bước 4: Nhấn Confirm
----> Thông tin mạng sẽ truyền tới Socket.

- Hẹn giờ trên app.

- Tự lưu giá trị vào bộ nhớ, khi khởi động lại sẽ không mất trạng thái.

- Bảo mật: Sử dụng giao thức MQTTS cần có CA mới có thể giải mã bản tin.

- Nhận biết dòng rò. Nếu có dòng rò sẽ tự ngắt.