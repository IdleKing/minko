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

#pragma once

#include "minko/Common.hpp"
#include "minko/component/AbstractAnimation.hpp"

namespace minko
{
	namespace component
	{
		class Animation: public AbstractAnimation
		{
			friend class MasterAnimation;

		public:
			typedef std::shared_ptr<Animation>						Ptr;

		private:
			typedef std::shared_ptr<animation::AbstractTimeline>	AbsTimelinePtr;
			typedef std::shared_ptr<MasterAnimation>				MasterAnimationPtr;

		private:
			const std::vector<AbsTimelinePtr>						_timelines;
			MasterAnimationPtr										_master;

		public:
			inline static
			Ptr
			create(const std::vector<AbsTimelinePtr>& timelines, bool isLooping = true)
			{
				Ptr ptr = std::shared_ptr<Animation>(new Animation(timelines, isLooping));

				ptr->initialize();

				return ptr;
			}

			inline
			uint
			numTimelines() const
			{
				return _timelines.size();
			}

			inline
			AbsTimelinePtr
			timeline(uint timelineId) const
			{
				return _timelines[timelineId];
			}

            inline
            const std::vector<AbsTimelinePtr>&
            timelines() const
            {
                return _timelines;
            }

		private:
			Animation(const std::vector<AbsTimelinePtr>&, bool isLooping);

			/*virtual*/
			void
			initialize();

			/*virtual*/
			void
			update();

			/*virtual*/
			void
			frameBeginHandler(std::shared_ptr<SceneManager> manager, float time, float deltaTime)
			{
				if (_master == nullptr)
					AbstractAnimation::frameBeginHandler(manager, time, deltaTime);
			}

			inline /*virtual*/
			void
			updateNextLabelIds(uint time)
			{
				if (_master == nullptr)
					AbstractAnimation::updateNextLabelIds(time);
			}

			inline /*virtual*/
			void 
			checkLabelHit(uint previousTime, uint newTime)
			{
				if (_master == nullptr)
					AbstractAnimation::checkLabelHit(previousTime, newTime);
			}
		};
	}
}