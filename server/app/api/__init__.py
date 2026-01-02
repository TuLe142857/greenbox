import cv2
from flask import Flask, Blueprint, render_template, Response

from service import ClassifyService
from .classify_api import classify_bp
from .model_api import models_bp

app_api = Blueprint('app_routes', __name__)
app_api.register_blueprint(classify_bp)
app_api.register_blueprint(models_bp)

@app_api.route("/")
def index():
    return render_template("index.html",
                           latest_time=ClassifyService.latest_timestamp, latest_rest=ClassifyService.latest_result)

@app_api.route("/latest")
def latest():
    latest_img = ClassifyService.latest_img
    if latest_img is None:
        return "No image yet", 404
    _, buffer = cv2.imencode('.jpg', latest_img)
    return Response(buffer.tobytes(), mimetype='image/jpeg')
