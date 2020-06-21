#ifndef SETTINGSGAMEPADCAPTUREDIALOG_H
#define SETTINGSGAMEPADCAPTUREDIALOG_H

#include <QDialog>

#include <chiaki/controller.h>

class SettingsGamepadCaptureDialog : public QDialog
{
	Q_OBJECT
public:
	explicit SettingsGamepadCaptureDialog(QWidget *parent = nullptr);
	ChiakiControllerButton GetCapturedButton() const noexcept {
		return button;
	}

signals:
	void ButtonCaptured(ChiakiControllerButton);

private:
	void SetCapturedButton(ChiakiControllerButton b);

	ChiakiControllerButton button = CHIAKI_CONTROLLER_BUTTON_NONE;
};

#endif // SETTINGSGAMEPADCAPTUREDIALOG_H
