#include "settingsgamepadcapturedialog.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "controllermanager.h"

SettingsGamepadCaptureDialog::SettingsGamepadCaptureDialog(QWidget *parent) :
    QDialog(parent)
{
	setWindowTitle(tr("Gamepad Capture"));

	auto root_layout = new QVBoxLayout(this);
	setLayout(root_layout);

	auto label = new QLabel(tr("Press any button to configure button or click close."));
	root_layout->addWidget(label);

	auto button = new QPushButton(tr("Close"), this);
	root_layout->addWidget(button);
	button->setAutoDefault(false);
	connect(button, &QPushButton::clicked, this, &QDialog::reject);

	const auto available_controllers = ControllerManager::GetInstance()->GetAvailableControllers();
	if(!available_controllers.isEmpty())
	{
		auto controller = ControllerManager::GetInstance()->OpenController(available_controllers[0]);
		if(controller)
		{
			connect(controller, &Controller::ButtonPressed, this, [this, controller](ChiakiControllerButton b) {
				SetCapturedButton(b);
				controller->deleteLater();
			});

		}
	}
}

void SettingsGamepadCaptureDialog::SetCapturedButton(ChiakiControllerButton b)
{
	button = b;
	emit ButtonCaptured(button);
	accept();
}
