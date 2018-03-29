/*
 * MIT License
 * 
 * Copyright (c) 2017 David Allen
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * AtmosphereRenderer.h
 * 
 * Created on: Mar 27, 2018
 *     Author: david
 */

#ifndef RENDERING_DEFERRED_ATMOSPHERERENDERER_H_
#define RENDERING_DEFERRED_ATMOSPHERERENDERER_H_

#include <common.h>
#include <Rendering/Renderer/RendererEnums.h>
#include <Rendering/Renderer/RendererObjects.h>
#include <Resources/ResourceManager.h>

class StarlightEngine;

class DensityProfileLayer
{
	public:
		DensityProfileLayer ()
				: DensityProfileLayer(0.0, 0.0, 0.0, 0.0, 0.0)
		{
		}
		DensityProfileLayer (double width, double exp_term, double exp_scale, double linear_term, double constant_term)
				: width(width), exp_term(exp_term), exp_scale(exp_scale), linear_term(linear_term), constant_term(constant_term)
		{
		}
		double width;
		double exp_term;
		double exp_scale;
		double linear_term;
		double constant_term;
};

class AtmosphereRenderer
{
	public:
		TextureView transmittanceTV;
		TextureView scatteringTV;
		TextureView irradianceTV;

		AtmosphereRenderer (StarlightEngine *enginePtr);
		virtual ~AtmosphereRenderer ();

		void init ();
		void destroy ();

		std::string getAtmosphericShaderLib ();

	private:

		Texture transmittanceTexture;
		Texture scatteringTexture;
		Texture irradianceTexture;

		bool destroyed;

		StarlightEngine *engine;
		std::string atmosphereShaderInclude;

		void loadPrecomputedTextures ();
		void loadScatteringSourceInclude ();

};

#endif /* RENDERING_DEFERRED_ATMOSPHERERENDERER_H_ */
