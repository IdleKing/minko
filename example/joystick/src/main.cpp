/*
Copyright (c) 2014 Aerys

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "minko/Minko.hpp"
#include "minko/MinkoSDL.hpp"

using namespace minko;
using namespace minko::math;
using namespace minko::component;

const uint WINDOW_WIDTH = 800;
const uint WINDOW_HEIGHT = 600;

std::unordered_map<input::Joystick::Ptr, scene::Node::Ptr> joystickToCube;
std::unordered_map<input::Joystick::Ptr, Signal<input::Joystick::Ptr, int, int>::Slot> joystickToButtonDownSlot;

Signal<AbstractCanvas::Ptr, input::Joystick::Ptr>::Slot joystickAdded;
Signal<AbstractCanvas::Ptr, input::Joystick::Ptr>::Slot joystickRemoved;

void
joystickButtonDownHandler(input::Joystick::Ptr joystick, int which, int buttonId)
{
    if (buttonId == 0)
        joystickToCube[joystick]->component<Transform>()->matrix()->appendTranslation(0.f, 0.f, -0.1f);
    if (buttonId == 1)
        joystickToCube[joystick]->component<Transform>()->matrix()->appendTranslation(0.f, 0.f, 0.1f);
    if (buttonId == 2)
        joystickToCube[joystick]->component<Transform>()->matrix()->appendTranslation(-.1f);
    if (buttonId == 3)
        joystickToCube[joystick]->component<Transform>()->matrix()->appendTranslation(.1f);
}

int
main(int argc, char** argv)
{
    auto canvas = Canvas::create("Minko Tutorial - Hello cube!", WINDOW_WIDTH, WINDOW_HEIGHT);
    auto sceneManager = SceneManager::create(canvas->context());

    // setup assets
    sceneManager->assets()->defaultOptions()->resizeSmoothly(true);
    sceneManager->assets()->defaultOptions()->generateMipmaps(true);
    sceneManager->assets()->queue("effect/Basic.effect");

    std::cout << "Plug a joystick and move the cube." << std::endl;

    auto complete = sceneManager->assets()->complete()->connect([&](file::AssetLibrary::Ptr assets)
    {
        auto root = scene::Node::create("root")
            ->addComponent(sceneManager);

        auto camera = scene::Node::create("camera")
            ->addComponent(Renderer::create(0x7f7f7fff))
            ->addComponent(PerspectiveCamera::create(
            (float) WINDOW_WIDTH / (float) WINDOW_HEIGHT, (float) PI * 0.25f, .1f, 1000.f)
            )
            ->addComponent(Transform::create(Matrix4x4::create()->lookAt(
            Vector3::create(), Vector3::create(0.f, 0.f, -5.f))));
        root->addChild(camera);

        joystickAdded = canvas->joystickAdded()->connect([&](AbstractCanvas::Ptr canvas, input::Joystick::Ptr joystick)
        {
            auto mesh = scene::Node::create("mesh")
                ->addComponent(Transform::create())
                ->addComponent(Surface::create(
                geometry::CubeGeometry::create(
                sceneManager->assets()->context()),
                material::BasicMaterial::create()->diffuseColor(math::Vector4::create(
                float(rand()) / float(RAND_MAX),
                float(rand()) / float(RAND_MAX),
                float(rand()) / float(RAND_MAX))),
                sceneManager->assets()->effect("effect/Basic.effect")
                )
                );

            joystickToCube[joystick] = mesh;
            joystickToButtonDownSlot[joystick] = joystick->joystickButtonDown()->connect(
                std::bind(
                &joystickButtonDownHandler,
                std::placeholders::_1,
                std::placeholders::_2,
                std::placeholders::_3
                )
                );

            root->addChild(mesh);
        });

        joystickRemoved = canvas->joystickRemoved()->connect([&](AbstractCanvas::Ptr canvas, input::Joystick::Ptr joystick)
        {
            root->removeChild(joystickToCube[joystick]);
            joystickToButtonDownSlot.erase(joystick);
            joystickToCube.erase(joystick);
        });

        auto cube = scene::Node::create("cube")
            ->addComponent(Transform::create(Matrix4x4::create()))
            ->addComponent(Surface::create(
            geometry::CubeGeometry::create(assets->context()),
            material::BasicMaterial::create()->diffuseColor(Vector4::create(1.f, 0.f, 1.f, 1.f)),
            assets->effect("effect/Basic.effect")
            ));
        root->addChild(cube);

        auto resized = canvas->resized()->connect([&](AbstractCanvas::Ptr canvas, uint w, uint h)
        {
            camera->component<PerspectiveCamera>()->aspectRatio((float) w / (float) h);
        });

        auto enterFrame = canvas->enterFrame()->connect([&](Canvas::Ptr canvas, float t, float dt)
        {
            cube->component<Transform>()->matrix()->prependRotationY(.01f);

            sceneManager->nextFrame(t, dt);
        });

        canvas->run();
    });

    sceneManager->assets()->load();

    return 0;
}

