# LOCAL SERVER FOR GREENBOX PROJECT

## Setup & Run

### For Linux
```commandline
    cd server
    python3 -m venv .venv
    source .venv/bin/activate
    pip install -r requirements.txt
    python run.py
    
```

### For Windows
- Windows có thể bị lỗi khi host server Flask dùng kèm numpy
- Để tránh lỗi nên dùng WSL hoặc docker 

#### Dùng venv(có thể lỗi):
```commandline
    cd server
    python -m venv .venv
    .\venv_name\Scripts\activate
    pip install -r requirements.txt
    python run.py
```

#### Dùng WSL:

#### Dùng Docker: tự build docker file đi :)