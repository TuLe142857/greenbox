# LOCAL SERVER FOR GREENBOX PROJECT

> [!NOTE]
> Thư mục /images sẽ lưu tất cả ảnh được gởi lên server từ ESP32-CAM  
> Thư mục /models là nơi server tìm và load model dạng <>.pt, tên `DEFAULT_MODEL` mặc định được 
> chỉ định trong config.py là `"final_model.pt"` nếu không tìm thấy thì server sẽ lấy models đầu
> tiên trong thư mục /models  
> Trong trang web demo có cho phép đổi models

## Setup & Run with Docker

### Build & run
```commandline
    cd server
    docker compose -p greenbox up -d --build  
```

### Show logs
```commandline
    docker compose -p greenbox logs -f
```

### Shutdown
```commandline
    docker compose -p greenbox down
```

## Setup & Run with Python venv
> [!WARNING]
> Chạy trực tiếp dễ lỗi dependences, lib versions, ...  
> Recommend chạy bằng Docker  
> File requirements.bak.txt là file được freeze từ python venv trên Fedora, nếu dùng requirements.txt
> lỗi mà vẫn ko muốn dùng docker thì lasttry  requirements.bak.txt thử :)

### Linux
```commandline
    cd server
    python3 -m venv .venv
    source .venv/bin/activate
    pip install -r requirements.txt
    python run.py
    
```

### For Windows

```commandline
    cd server
    python -m venv .venv
    .\venv\Scripts\activate
    pip install -r requirements.txt
    python run.py
```