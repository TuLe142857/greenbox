from flask import Blueprint, request, jsonify
import cv2
import numpy as np

from service import ClassifyService

classify_bp = Blueprint('classify_api', __name__, url_prefix="/classify")

@classify_bp.route("/", methods=["POST"])
def classify():
    data = request.data
    if len(data) == 0:
        return "No data received", 400
    img = cv2.imdecode(np.frombuffer(data, np.uint8), cv2.IMREAD_COLOR)
    class_id = ClassifyService.classify(img)
    if class_id is None:
        return "No model found", 404
    return str(class_id), 200