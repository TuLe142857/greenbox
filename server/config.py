import os
class Config:
    DEFAULT_MODEL = "final_model.pt"
    MODEL_DIRECTORY = os.path.join(os.path.dirname(os.path.abspath(__file__)), "models")
    IMAGE_DIRECTORY = os.path.join(os.path.dirname(os.path.abspath(__file__)), "images")
