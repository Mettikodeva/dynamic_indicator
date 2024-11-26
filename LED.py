import sys
from PySide6.QtWidgets import QApplication, QLabel

app = QApplication(sys.argv)
print(app.arguments()[-1])
label = QLabel("Hello World!")
label.show()
app.exec()