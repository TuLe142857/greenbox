import cv2
from flask import current_app
import os
import time
from ultralytics import YOLO

class ClassifyService:
    MODEL = None
    CURRENT_MODEL_NAME = None

    latest_img = None
    latest_timestamp = None
    latest_result = None



    @staticmethod
    def get_all_models()->list[str]:
        model_directory = current_app.config["MODEL_DIRECTORY"]
        entries = os.listdir(model_directory)
        return [f for f in entries if os.path.isfile(os.path.join(model_directory, f))]

    @staticmethod
    def load_model(model_name):
        model_directory = current_app.config["MODEL_DIRECTORY"]
        model_path = os.path.join(model_directory, model_name)
        if not os.path.isfile(model_path):
            current_app.logger.warning(f"No model found for {model_name} - {model_path}")
            print(f"No model found for {model_name} - {model_path}")
            return False
        ClassifyService.MODEL = YOLO(str(model_path))
        if ClassifyService.MODEL is None:
            current_app.logger.warning(f"No model found for {model_name} = {model_path}")
            print(f"No model found for {model_name} - {model_path}")
            return False
        ClassifyService.CURRENT_MODEL_NAME = model_name
        current_app.logger.info(f"Model {model_name} loaded")
        print(f"load model ok {model_path}")
        print(ClassifyService.MODEL.names)
        return True

    @staticmethod
    def load_default_model():
        default_model = current_app.config.get('DEFAULT_MODEL')
        if default_model in ClassifyService.get_all_models():
            return ClassifyService.load_model(default_model)
        else:
            all_models = ClassifyService.get_all_models()
            if len(all_models) > 0:
                return ClassifyService.load_model(all_models[0])
            else:
                return False

    @staticmethod
    def classify(img, conf: float = 0.5) -> None | int:
        if not ClassifyService.MODEL:
            if not ClassifyService.load_default_model():
                current_app.logger.warning("Model not loaded")
                return None
        model = ClassifyService.MODEL
        results = model(img, conf=conf)

        if not results or results[0].boxes is None or len(results[0].boxes) == 0:
            ClassifyService.latest_img = img
            ClassifyService.latest_timestamp = time.time()
            ClassifyService.latest_result = -1
            cv2.imwrite(f"{os.path.join(current_app.config.get('IMAGE_DIRECTORY'), f'{time.time()}.png')}", img)
            return -1

        best_box = max(
            results[0].boxes,
            key=lambda b: float(b.conf[0])
        )
        class_id = int(best_box.cls[0])

        ClassifyService.latest_img = results[0].plot()
        ClassifyService.latest_timestamp = time.time()
        ClassifyService.latest_result = class_id
        cv2.imwrite(f"{os.path.join(current_app.config.get('IMAGE_DIRECTORY'), f'{time.time()}.png')}", img)
        return class_id