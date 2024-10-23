#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <String.h>

// Khởi tạo LCD với địa chỉ I2C 0x27 cho ESP32
LiquidCrystal_I2C lcd(0x27, 16, 2);

typedef void (*date_time_event)();

// Khai báo chân cảm biến hồng ngoại IR
const int irPin = 13; // Cảm biến hồng ngoại kết nối với GPIO 13

class BaseDateTime {
protected:
    int vals[3];
    int limit(int v, int min, int max) {
        if (v < min) return min;
        return v > max ? max : v;
    }

public:
    date_time_event on_limit;
    BaseDateTime() : on_limit(0) {
    }
    
    int operator[](int index) const { return vals[index]; }
};

class Time : public BaseDateTime {
public:
    int Hour() const { return vals[0]; }
    int Minute() const { return vals[1]; }
    int Second() const { return vals[2]; }

    Time(int hour = 0, int minute = 0, int second = 0) {
        vals[0] = limit(hour, 0, 23);
        vals[1] = limit(minute, 0, 59);
        vals[2] = limit(second, 0, 59);
    }

    Time& operator++() {
        if (++vals[2] == 60) {
            vals[2] = 0;
            if (++vals[1] == 60) {
                vals[1] = 0;
                if (++vals[0] == 24) {
                    vals[0] = 0;
                    if (on_limit) on_limit(); // Gọi sự kiện khi đồng hồ chuyển sang ngày mới
                }
            }
        }
        return *this;
    }
};

class Date : public BaseDateTime {
public:
    int Day() const { return vals[2]; }
    int Month() const { return vals[1]; }
    int Year() const { return vals[0]; }

    int DayOfMonth() const {
        switch (Month()) {
            case 2: return (Year() & 3) ? 28 : 29; // Năm nhuận
            case 4: case 6: case 9: case 11: return 30; // Các tháng có 30 ngày
        }
        return 31; // Các tháng còn lại có 31 ngày
    }

    Date(int year = 1, int month = 1, int day = 1) {
        vals[0] = year;
        vals[1] = limit(month, 1, 12);
        vals[2] = limit(day, 1, DayOfMonth());
    }

    Date& operator++() {
        if (++vals[2] > DayOfMonth()) {
            vals[2] = 1;
            if (++vals[1] > 12) {
                vals[1] = 1;
                ++vals[0];
                if (on_limit) on_limit(); // Gọi sự kiện khi tháng chuyển sang tháng mới
            }
        }
        return *this;
    }
};

// Hàm định dạng với số 0 đứng trước
String FormatWithLeadingZero(int a) {
    if (a < 10 && a >= 0) { // Kiểm tra nếu a có một chữ số
        return "0" + String(a); // Thêm số 0 vào trước
    }
    return String(a); // Trả về giá trị bình thường nếu đã có hai chữ số
}

// Khởi tạo đối tượng ngày và giờ
Date a {2004, 12, 31};
Time b {23, 59, 56};

void ShowDate(const Date& a) {
    lcd.setCursor(0, 0);  // Đặt con trỏ tại dòng đầu tiên
    lcd.print(FormatWithLeadingZero(a.Day()));
    lcd.print("/");
    lcd.print(FormatWithLeadingZero(a.Month()));
    lcd.print("/");
    lcd.print(a.Year());
}
void ShowTime(const Time& b) {
    lcd.setCursor(0, 1);  // Đặt con trỏ tại dòng thứ hai
    lcd.print(FormatWithLeadingZero(b.Hour()));
    lcd.print(":");
    lcd.print(FormatWithLeadingZero(b.Minute()));
    lcd.print(":");
    lcd.print(FormatWithLeadingZero(b.Second()));
}
void EndDay() {
    ShowDate(++a); // Nhảy sang ngày mới và hiển thị
}
void setup() {
    lcd.init();
    lcd.backlight();
    lcd.clear();
    ShowDate(a);  // Hiển thị ngày ban đầu
    ShowTime(b);  // Hiển thị thời gian ban đầu

    // Thiết lập sự kiện cho đồng hồ
    b.on_limit = EndDay; // Gán hàm EndDay vào sự kiện on_limit

    pinMode(irPin, INPUT); // Cấu hình chân cảm biến IR là INPUT
}
void loop() {
    // Kiểm tra tín hiệu từ cảm biến IR
    int irState = digitalRead(irPin);
    if (irState == LOW) { // Nếu phát hiện vật cản (LOW là không có phản hồi từ IR)
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Detecting Object");
    } else {
        lcd.clear();  // Xóa màn hình để cập nhật lại
        ShowDate(a);  // Hiển thị ngày
        ShowTime(++b);  // Cập nhật thời gian (tăng giây)
        delay(1000);
    }
    delay(1000);  // Đợi 1 giây trước khi cập nhật lại
}
