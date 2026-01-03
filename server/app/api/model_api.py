from flask import Blueprint, jsonify, current_app, request
import os
from service import ClassifyService


models_bp = Blueprint('model_api', __name__, url_prefix="/models")

@models_bp.route("/", methods=["GET"])
def get_all_models():
    return jsonify(ClassifyService.get_all_models()), 200


@models_bp.route("/current", methods=["GET"])
def get_current():
    return jsonify(ClassifyService.CURRENT_MODEL_NAME), 200

@models_bp.route("/select", methods=["POST"])
def select_model():
    data = request.get_json()
    if not data or 'model_name' not in data:
        return jsonify({"message": "Missing 'model_name' in payload"}), 400
    model_name = data['model_name']
    if not model_name in ClassifyService.get_all_models():
        return jsonify({"message": "Model not found"}), 404
    ClassifyService.load_model(model_name)
    return jsonify({"message": "Success"}), 200