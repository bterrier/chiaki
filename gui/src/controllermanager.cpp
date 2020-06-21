/*
 * This file is part of Chiaki.
 *
 * Chiaki is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Chiaki is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Chiaki.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <controllermanager.h>

#include <QCoreApplication>
#include <QMessageBox>
#include <QByteArray>
#include <QTimer>

#ifdef CHIAKI_GUI_ENABLE_SDL_GAMECONTROLLER
#include <SDL.h>
#endif

static ControllerManager *instance = nullptr;

#define UPDATE_INTERVAL_MS 4

ControllerManager *ControllerManager::GetInstance()
{
	if(!instance)
		instance = new ControllerManager(qApp);
	return instance;
}

ControllerManager::ControllerManager(QObject *parent)
	: QObject(parent)
{
#ifdef CHIAKI_GUI_ENABLE_SDL_GAMECONTROLLER
	SDL_SetMainReady();
	if(SDL_Init(SDL_INIT_GAMECONTROLLER) < 0)
	{
		const char *err = SDL_GetError();
		QMessageBox::critical(nullptr, "SDL Init", tr("Failed to initialized SDL Gamecontroller: %1").arg(err ? err : ""));
	}

	auto timer = new QTimer(this);
	connect(timer, &QTimer::timeout, this, &ControllerManager::HandleEvents);
	timer->start(UPDATE_INTERVAL_MS);
#endif

	UpdateAvailableControllers();
}

ControllerManager::~ControllerManager()
{
#ifdef CHIAKI_GUI_ENABLE_SDL_GAMECONTROLLER
	SDL_Quit();
#endif
}

void ControllerManager::UpdateAvailableControllers()
{
#ifdef CHIAKI_GUI_ENABLE_SDL_GAMECONTROLLER
	QSet<SDL_JoystickID> current_controllers;
	for(int i=0; i<SDL_NumJoysticks(); i++)
	{
		if(!SDL_IsGameController(i))
			continue;
		current_controllers.insert(SDL_JoystickGetDeviceInstanceID(i));
	}

	if(current_controllers != available_controllers)
	{
		available_controllers = current_controllers;
		emit AvailableControllersUpdated();
	}
#endif
}

void ControllerManager::HandleEvents()
{
#ifdef CHIAKI_GUI_ENABLE_SDL_GAMECONTROLLER
	SDL_Event event;
	while(SDL_PollEvent(&event))
	{
		switch(event.type)
		{
			case SDL_JOYDEVICEADDED:
			case SDL_JOYDEVICEREMOVED:
				UpdateAvailableControllers();
				break;
			case SDL_CONTROLLERBUTTONUP:
			case SDL_CONTROLLERBUTTONDOWN:
			    ControllerEvent(event.cbutton);
				break;
			case SDL_CONTROLLERAXISMOTION:
			    //ControllerEvent(event.caxis.which);
				break;
		}
	}
#endif
}

void ControllerManager::ControllerEvent(SDL_ControllerButtonEvent event)
{
	auto it = open_controllers.find(event.which);
	if (it != open_controllers.end())
	it.value()->ButtonEvent(event);
}

QList<int> ControllerManager::GetAvailableControllers()
{
#ifdef CHIAKI_GUI_ENABLE_SDL_GAMECONTROLLER
	return available_controllers.values();
#else
	return {};
#endif
}

Controller *ControllerManager::OpenController(int device_id)
{
	if(open_controllers.contains(device_id))
		return nullptr;
	auto controller = new Controller(device_id, this);
	open_controllers[device_id] = controller;
	return controller;
}

void ControllerManager::ControllerClosed(Controller *controller)
{
	open_controllers.remove(controller->GetDeviceID());
}

Controller::Controller(int device_id, ControllerManager *manager) : QObject(manager)
{
	this->id = device_id;
	this->manager = manager;

#ifdef CHIAKI_GUI_ENABLE_SDL_GAMECONTROLLER
	controller = nullptr;
	for(int i=0; i<SDL_NumJoysticks(); i++)
	{
		if(SDL_JoystickGetDeviceInstanceID(i) == device_id)
		{
			controller = SDL_GameControllerOpen(i);
			break;
		}
	}
#endif
}

Controller::~Controller()
{
#ifdef CHIAKI_GUI_ENABLE_SDL_GAMECONTROLLER
	if(controller)
		SDL_GameControllerClose(controller);
#endif
	manager->ControllerClosed(this);
}

void Controller::UpdateState()
{
	emit StateChanged();
}

static ChiakiControllerButton sdl2chiaki(SDL_GameControllerButton sdlButton) {
	switch (sdlButton) {
	case SDL_CONTROLLER_BUTTON_A:
		return CHIAKI_CONTROLLER_BUTTON_CROSS;
	case SDL_CONTROLLER_BUTTON_B:
		return CHIAKI_CONTROLLER_BUTTON_MOON;
	case SDL_CONTROLLER_BUTTON_X:
		return CHIAKI_CONTROLLER_BUTTON_BOX;
	case SDL_CONTROLLER_BUTTON_Y:
		return CHIAKI_CONTROLLER_BUTTON_PYRAMID;
	}
}

void Controller::ButtonEvent(SDL_ControllerButtonEvent event)
{
	if (event.type == SDL_CONTROLLERBUTTONDOWN)
		emit ButtonPressed(sdl2chiaki(SDL_GameControllerButton(event.button)));
	UpdateState();
}

ChiakiControllerButton Controller::mapped(ChiakiControllerButton input) const
{
	return mapping.value(input, input);
}

bool Controller::IsConnected()
{
#ifdef CHIAKI_GUI_ENABLE_SDL_GAMECONTROLLER
	return controller && SDL_GameControllerGetAttached(controller);
#else
	return false;
#endif
}

int Controller::GetDeviceID()
{
#ifdef CHIAKI_GUI_ENABLE_SDL_GAMECONTROLLER
	return id;
#else
	return -1;
#endif
}

QString Controller::GetName()
{
#ifdef CHIAKI_GUI_ENABLE_SDL_GAMECONTROLLER
	if(!controller)
		return QString();
	return SDL_GameControllerName(controller);
#else
	return QString();
#endif
}

ChiakiControllerState Controller::GetState()
{
	ChiakiControllerState state = {};
#ifdef CHIAKI_GUI_ENABLE_SDL_GAMECONTROLLER
	if(!controller)
		return state;

	state.buttons |= SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_A) ? mapped(CHIAKI_CONTROLLER_BUTTON_CROSS) : 0;
	state.buttons |= SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_B) ? mapped(CHIAKI_CONTROLLER_BUTTON_MOON) : 0;
	state.buttons |= SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_X) ? mapped(CHIAKI_CONTROLLER_BUTTON_BOX) : 0;
	state.buttons |= SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_Y) ? mapped(CHIAKI_CONTROLLER_BUTTON_PYRAMID) : 0;
	state.buttons |= SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT) ? mapped(CHIAKI_CONTROLLER_BUTTON_DPAD_LEFT) : 0;
	state.buttons |= SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT) ? mapped(CHIAKI_CONTROLLER_BUTTON_DPAD_RIGHT) : 0;
	state.buttons |= SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_UP) ? mapped(CHIAKI_CONTROLLER_BUTTON_DPAD_UP) : 0;
	state.buttons |= SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN) ? mapped(CHIAKI_CONTROLLER_BUTTON_DPAD_DOWN) : 0;
	state.buttons |= SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_LEFTSHOULDER) ? mapped(CHIAKI_CONTROLLER_BUTTON_L1) : 0;
	state.buttons |= SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) ? mapped(CHIAKI_CONTROLLER_BUTTON_R1) : 0;
	state.buttons |= SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_LEFTSTICK) ? mapped(CHIAKI_CONTROLLER_BUTTON_L3) : 0;
	state.buttons |= SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_RIGHTSTICK) ? mapped(CHIAKI_CONTROLLER_BUTTON_R3) : 0;
	state.buttons |= SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_START) ? mapped(CHIAKI_CONTROLLER_BUTTON_OPTIONS) : 0;
	state.buttons |= SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_BACK) ? mapped(CHIAKI_CONTROLLER_BUTTON_TOUCHPAD) : 0;
	state.buttons |= SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_GUIDE) ? mapped(CHIAKI_CONTROLLER_BUTTON_PS) : 0;
	state.l2_state = (uint8_t)(SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT) >> 7);
	state.r2_state = (uint8_t)(SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) >> 7);
	state.left_x = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX);
	state.left_y = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY);
	state.right_x = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTX);
	state.right_y = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTY);

#endif
	return state;
}

void Controller::SetMapping(const QHash<ChiakiControllerButton, ChiakiControllerButton> &map)
{
	mapping = map;
}
