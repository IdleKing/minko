/*
Copyright (c) 2013 Aerys

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

#include <time.h>

#include "minko/ParticlesCommon.hpp"
#include "minko/component/AbstractComponent.hpp"
#include "minko/geometry/ParticlesGeometry.hpp"
#include "minko/particle/ParticleData.hpp"

namespace minko
{
	namespace component
	{
		class ParticleSystem :
			public AbstractComponent,
			public std::enable_shared_from_this<ParticleSystem>
		{
		public:
			typedef std::shared_ptr<ParticleSystem>	Ptr;

		private:
			typedef std::shared_ptr<render::AbstractContext>					AbstractContextPtr;
			typedef std::shared_ptr<file::AssetLibrary>							AssetLibraryPtr;

			typedef std::shared_ptr<Transform>									TransformPtr;

			typedef std::shared_ptr<scene::NodeSet>								NodeSetPtr;
			typedef std::shared_ptr<scene::Node>								NodePtr;
			typedef std::shared_ptr<AbstractComponent>							AbsCompPtr;
			typedef std::shared_ptr<Renderer>									RendererPtr;

			typedef std::shared_ptr<Surface>									SurfacePtr;
			typedef std::shared_ptr<geometry::ParticlesGeometry>				GeometryPtr;
			typedef std::shared_ptr<data::ParticlesProvider>					ParticlesProviderPtr;
			typedef std::shared_ptr<render::Effect>								EffectPtr;

			typedef std::shared_ptr<particle::shape::EmitterShape>				ShapePtr;
			typedef std::shared_ptr<particle::sampler::Sampler<float> >			FloatSamplerPtr;
			typedef std::shared_ptr<particle::modifier::IParticleInitializer>	IInitializerPtr;
			typedef std::shared_ptr<particle::modifier::IParticleUpdater>		IUpdaterPtr;
			typedef std::shared_ptr<particle::modifier::IParticleModifier>		ModifierPtr;

		private:
			class ParticleDistanceToCameraComparison
			{
			public:
				component::ParticleSystem*	system;

				inline
				bool
				operator() (unsigned int p1Index, unsigned int p2Index)
				{
					return system->getParticleSquaredDistanceToCamera(p1Index)
						 > system->getParticleSquaredDistanceToCamera(p2Index);
				};
			};

		private:
			static const unsigned int 								    COUNT_LIMIT;

			GeometryPtr									                _geometry;
			ParticlesProviderPtr						                _material;
			EffectPtr									                _effect;
			SurfacePtr									                _surface;

			TransformPtr								                _toWorld;

			unsigned int												_countLimit;
			unsigned int								                _maxCount;
			//unsigned int								                _liveCount;
			unsigned int								                _previousLiveCount;
			std::vector<IInitializerPtr> 				                _initializers;
			std::vector<IUpdaterPtr> 					                _updaters;
			std::vector<particle::ParticleData>			                _particles;
			std::vector<unsigned int>					                _particleOrder;
			std::vector<float>							                _particleDistanceToCamera;

			bool										                _isInWorldSpace;
			float 										                _localToWorld[16];
			bool										                _isZSorted;
			float 										                _cameraCoords[3];
			ParticleDistanceToCameraComparison			                _comparisonObject;
			bool										                _useOldPosition;

			float										                _rate;
			FloatSamplerPtr								                _lifetime;
			ShapePtr									                _shape;
			particle::StartDirection					                _emissionDirection;
			FloatSamplerPtr 							                _emissionVelocity;

			float										                _createTimer;

			int											                _format;

			float										                _updateStep;
			bool										                _playing;
			bool										                _emitting;
			float										                _time;

			Signal<std::shared_ptr<SceneManager>, float, float>::Slot	_frameBeginSlot;
			Signal<AbsCompPtr, NodePtr>::Slot			                _targetAddedSlot;
			Signal<AbsCompPtr, NodePtr>::Slot			                _targetRemovedSlot;
			Signal<NodePtr, NodePtr, NodePtr>::Slot		                _addedSlot;
			Signal<NodePtr, NodePtr, NodePtr>::Slot		                _removedSlot;
			Signal<NodePtr, NodePtr, NodePtr>::Slot		                _rootDescendantAddedSlot;
			Signal<NodePtr, NodePtr, NodePtr>::Slot		                _rootDescendantRemovedSlot;
			Signal<NodePtr, NodePtr, AbsCompPtr>::Slot	                _componentAddedSlot;
			Signal<NodePtr, NodePtr, AbsCompPtr>::Slot	                _componentRemovedSlot;

		public:
			static
			Ptr
			create(AssetLibraryPtr			assets,
				   float					rate,
				   FloatSamplerPtr			lifetime,
				   ShapePtr					shape,
				   particle::StartDirection	emissionDirection,
				   FloatSamplerPtr 			emissionVelocity)
			{
				Ptr ptr = std::shared_ptr<ParticleSystem> (new ParticleSystem(
					assets,
					rate,
					lifetime,
					shape,
					emissionDirection,
					emissionVelocity
				));

				ptr->initialize();

				return ptr;
			};

			inline
			ParticlesProviderPtr
			material() const
			{
				return _material;
			}

			inline
			Ptr
			rate(float value)
			{
				_rate =  1.0f / value;

				updateMaxParticlesCount();

                return shared_from_this();
			}

			inline
			Ptr
			lifetime(FloatSamplerPtr value)
			{
				_lifetime = value;

				updateMaxParticlesCount();

                return shared_from_this();
			}

			inline
			Ptr
			shape(ShapePtr value)
			{
				_shape = value;

                return shared_from_this();
			}

			inline
			Ptr
			emissionDirection(particle::StartDirection value)
			{
				_emissionDirection = value;

                return shared_from_this();
			}

			inline
			Ptr
			emissionVelocity(FloatSamplerPtr value)
			{
				_emissionVelocity = value;

                return shared_from_this();
			}

			inline
			void
			updateRate(unsigned int updatesPerSecond)
			{
				_updateStep = 1.0f / (float)updatesPerSecond;
			}

			inline
			Ptr
			playing(bool value)
			{
                _playing = true;
				//if (value != _playing)
				//{
				//	_playing = value;
				//	if (_playing)
				//		_previousClock = clock();
				//}

                return shared_from_this();
			}

			inline
			Ptr
			emitting(bool value)
			{
				_emitting = value;

                return shared_from_this();
			}

			inline
			Ptr
			play()
			{
				reset();

				return playing(true);
			}

			inline
			Ptr
			stop()
			{
				reset();
				playing(false);
				updateVertexBuffer();

                return shared_from_this();
			}

			inline
			Ptr
			pause()
			{
				return playing(false);
			}

			inline
			Ptr
			resume()
			{
				return playing(true);
			}

		public:
			void
			updateSystem(float	timeStep,
					     bool	emit);

			void
			fastForward(float           time,
						unsigned int    updatesPerSecond = 0);

			void
			reset();

		public:
			Ptr
			add(ModifierPtr);

			Ptr
			remove(ModifierPtr);

			bool
			has(ModifierPtr) const;

		public:
            Ptr
            isInWorldSpace(bool);

			Ptr
			isZSorted(bool);

			Ptr
			useOldPosition(bool);

        /**
			inline
			void
			isInWorldSpace(bool value)
			{
				if (_isInWorldSpace == value)
					return;

				_isInWorldSpace = value;

				if (_isInWorldSpace)
					_material->set("particles.worldspace",	true);
				else
					_material->unset("particles.worldspace");
			};
            **/

			inline
			float*
			localToWorld() 
			{
				return _localToWorld;
			}


			inline
			float*
			cameraPos()
			{
				return _cameraCoords;
			};



			inline
			float
			getParticleSquaredDistanceToCamera(unsigned int particleIndex)
			{
				return _particleDistanceToCamera[particleIndex];
			};

			void
			updateParticleDistancesToCamera();

		public:
			inline
			unsigned int
			maxParticlesCount() const
			{
				return _maxCount;
			};

			void
			updateMaxParticlesCount();

			inline
			void
			countLimit(unsigned int value)
			{
				if (value > COUNT_LIMIT)
					throw std::length_error("A particle system can have a maximum of " + std::to_string(COUNT_LIMIT) + " particles.");

				_countLimit = value;

				updateMaxParticlesCount();
			};

			inline
			std::vector<particle::ParticleData>&
			getParticles()
			{
				return _particles;
			};

			void
			createParticle(unsigned int 						particleIndex,
						   const particle::shape::EmitterShape&	emitter,
						   float								timeLived);

			//void
			//killParticle(unsigned int							particleIndex);

		public:
			inline
			unsigned int
			formatFlags() const
			{
				return _format;
			};

			unsigned int
			updateVertexFormat();

		private:
			void
			resizeParticlesVector();

			void
			addComponents(unsigned int components, bool blockVSInit = false);

			inline
			void
			setInVertexBuffer(float* ptr, unsigned int offset, float value)
			{
                unsigned int idx = offset;
                for (unsigned int i = 0; i < 4; ++i)
                {
                    ptr[idx] = value;
                    idx += _geometry->vertexSize();
                }
			};

			void
			updateVertexBuffer();

		protected:
			ParticleSystem(AssetLibraryPtr,
						   float					rate,
						   FloatSamplerPtr			lifetime,
						   ShapePtr,
						   particle::StartDirection	emissionDirection,
						   FloatSamplerPtr 			emissionVelocity);

			void
			initialize();

			void
			targetAddedHandler(AbsCompPtr ctrl, NodePtr target);

			void
			targetRemovedHandler(AbsCompPtr ctrl, NodePtr target);

			void
			findSceneManager();

			void
			frameBeginHandler(std::shared_ptr<SceneManager>, float, float);
		};
	}
}