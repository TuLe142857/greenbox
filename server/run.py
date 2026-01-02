from app import create_app, ClassifyService
from config import Config
import os
if __name__ == '__main__':
    app = create_app(Config)
    os.makedirs(app.config.get('MODEL_DIRECTORY'), exist_ok=True)
    os.makedirs(app.config.get('IMAGE_DIRECTORY'), exist_ok=True)
    app.run(host="0.0.0.0", port=5000, debug=True)