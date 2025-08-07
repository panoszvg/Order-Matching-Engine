from flask import Flask
from order_router import order_bp
from snapshot_router import snapshot_bp

app = Flask(__name__)
app.register_blueprint(order_bp, url_prefix='/api')
app.register_blueprint(snapshot_bp, url_prefix='/api')

if __name__ == "__main__":
    app.run(debug=True, port=5000)
