from flask import Flask, render_template
from flask_cors import CORS
from .api import app_api
from .service import ClassifyService

def create_app(config_class):
    app = Flask(__name__)
    app.config.from_object(config_class)
    app.register_blueprint(app_api)
    CORS(app, resources={r"/api/*": {"origins": "*"}})
    return app