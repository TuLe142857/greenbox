from flask import Flask, request, Response
import cv2
import numpy as np
import random
from datetime import  datetime
import time
import os
from classify_service import ClassifyService
app = Flask(__name__)
latest_img = None
lastest_time = None
lastest_rest = None
classifyService = ClassifyService()
@app.route("/")
def root():
    return f"""
    <html>
    <head>
        <title>GreenBox - Phân loại rác thông minh</title>
    </head>
    <body>
        <h1>GreenBox</h1>
        <h3>Phân loại rác thông minh</h3>
        <p><strong>Ảnh mới nhất nhận từ ESP32-CAM:</strong></p>
        <p>result: {lastest_rest}</p>
        <p>Thời gian: {lastest_time}</p>
        # <img src="/latest" alt="Latest Image" />
    </body>
    </html>
    """

@app.route("/latest")
def latest():
    global latest_img
    if latest_img is None:
        return "No image yet", 404
    # encode to JPEG
    _, buffer = cv2.imencode('.jpg', latest_img)
    return Response(buffer.tobytes(), mimetype='image/jpeg')


WASTE_TYPES = ['0', '1', '2', '3']
@app.route("/classify", methods=['POST'])
def post_img():
    data = request.data
    if len(data) == 0:
        return "No data received", 400

    global latest_img
    global lastest_time
    latest_img = cv2.imdecode(np.frombuffer(data, np.uint8), cv2.IMREAD_COLOR)
    lastest_time = datetime.now()

    os.makedirs("imgs", exist_ok=True)
    cv2.imwrite(f"imgs/{time.time()}.png", latest_img)
    global lastest_rest
    result = classifyService.classify(latest_img)

    if result is None:
        lastest_rest = "-1"
        return lastest_rest, 200
    rest, annotated_img = result
    latest_img = annotated_img
    print(rest)
    if(rest == None): rest=-1
    lastest_rest = str(rest)
    return lastest_rest,  200


if __name__ == '__main__':
    app.run(host='0.0.0.0', debug=True)