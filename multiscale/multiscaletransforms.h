#ifndef MULTI_SCALE_TRANSFORMS_H
#define MULTI_SCALE_TRANSFORMS_H

#include <cmath>
#include <initializer_list>

#include <aocommon/uvector.h>

class MultiScaleTransforms
{
public:
	enum Shape { TaperedQuadraticShape, GaussianShape };
	
	MultiScaleTransforms(class FFTWManager& fftwManager, size_t width, size_t height, Shape shape) :
	_fftwManager(fftwManager),
	_width(width), _height(height), _shape(shape)
	{ }
	
	void PrepareTransform(double* kernel, double scale);
	void FinishTransform(double* image, const double* kernel);
	
	void Transform(double* image, double* scratch, double scale)
	{
		aocommon::UVector<double*> images(1, image);
		Transform(images, scratch, scale);
	}
	
	void Transform(const aocommon::UVector<double*>& images, double* scratch, double scale);
	
	size_t Width() const { return _width; }
	size_t Height() const { return _height; }
	
	static double KernelIntegratedValue(double scaleInPixels, size_t maxN, Shape shape)
	{
		size_t n;
		aocommon::UVector<double> kernel;
		MakeShapeFunction(scaleInPixels, kernel, n, maxN, shape);
		
		double value = 0.0;
		for(aocommon::UVector<double>::const_iterator x=kernel.begin(); x!=kernel.end(); ++x)
			value += *x;
		
		return value;
	}
	
	static double KernelPeakValue(double scaleInPixels, size_t maxN, Shape shape)
	{
		size_t n;
		aocommon::UVector<double> kernel;
		MakeShapeFunction(scaleInPixels, kernel, n, maxN, shape);
		return kernel[n/2 + (n/2)*n];
	}
	
	static void AddShapeComponent(double* image, size_t width, size_t height, double scaleSizeInPixels, size_t x, size_t y, double gain, Shape shape)
	{
		size_t n;
		aocommon::UVector<double> kernel;
		MakeShapeFunction(scaleSizeInPixels, kernel, n, std::min(width,height), shape);
		int left;
		if(x > n/2)
			left = x - n/2;
		else
			left = 0;
		int top;
		if(y > n/2)
			top = y - n/2;
		else
			top = 0;
		size_t right = std::min(x + (n+1)/2, width);
		size_t bottom = std::min(y + (n+1)/2, height);
		for(size_t yi=top; yi!=bottom; ++yi)
		{
			double* imagePtr = &image[yi * width];
			const double* kernelPtr = &kernel.data()[(yi+n/2-y)*n + left+n/2-x];
			for(size_t xi=left; xi!=right; ++xi)
			{
				imagePtr[xi] += *kernelPtr * gain;
				++kernelPtr;
			}
		}
	}
	
	static void MakeShapeFunction(double scaleSizeInPixels, aocommon::UVector<double>& output, size_t& n, size_t maxN, Shape shape)
	{
		switch(shape)
		{
			default:
			case TaperedQuadraticShape:
				makeTaperedQuadraticShapeFunction(scaleSizeInPixels, output, n);
				break;
			case GaussianShape:
				makeGaussianFunction(scaleSizeInPixels, output, n, maxN);
				break;
		}
	}
	
	void MakeShapeFunction(double scaleSizeInPixels, aocommon::UVector<double>& output, size_t& n)
	{
		MakeShapeFunction(scaleSizeInPixels, output, n, std::min(_width, _height), _shape);
	}
	
	static double GaussianSigma(double scaleSizeInPixels)
	{
		return scaleSizeInPixels * (3.0 / 16.0);
	}
private:
	class FFTWManager& _fftwManager;
	size_t _width, _height;
	enum Shape _shape;
	
	static size_t taperedQuadraticKernelSize(double scaleInPixels)
	{
		return size_t(ceil(scaleInPixels*0.5)*2.0)+1;
	}
	
	static void makeTaperedQuadraticShapeFunction(double scaleSizeInPixels, aocommon::UVector<double>& output, size_t& n)
	{
		n = taperedQuadraticKernelSize(scaleSizeInPixels);
		output.resize(n * n);
		taperedQuadraticShapeFunction(n, output, scaleSizeInPixels);
	}
	
	static void makeGaussianFunction(double scaleSizeInPixels, aocommon::UVector<double>& output, size_t& n, size_t maxN)
	{
		double sigma = GaussianSigma(scaleSizeInPixels);
		
		//n = maxN;
		//if((n%2) == 0 && n > 0) --n;
		
		n = int(ceil(sigma * 12.0 / 2.0)) * 2 + 1; // bounding box of 12 sigma
		if(n > maxN)
		{
			n = maxN;
			if((n%2) == 0 && n > 0) --n;
		}
		if(n < 1)
			n = 1;
		if(sigma == 0.0)
		{
			sigma = 1.0;
			n = 1;
		}
		output.resize(n * n);
		const double mu = int(n/2);
		const double twoSigmaSquared = 2.0 * sigma * sigma;
		double sum = 0.0;
		double* outputPtr = output.data();
		aocommon::UVector<double> gaus(n);
		for(int i=0; i!=int(n) ;++i)
		{
			double vI = double(i) - mu;
			gaus[i] = exp(-vI*vI / twoSigmaSquared);
		}
		for(int y=0; y!=int(n); ++y)
		{
			for(int x=0; x!=int(n) ;++x)
			{
				*outputPtr = gaus[x] * gaus[y];
				sum += *outputPtr;
				++outputPtr;
			}
		}
		double normFactor = 1.0 / sum;
		for(double &v : output)
			v *= normFactor;
	}
	
	static void taperedQuadraticShapeFunction(size_t n, aocommon::UVector<double>& output2d, double scaleSizeInPixels)
	{
		if(scaleSizeInPixels == 0.0)
			output2d[0] = 1.0;
		else {
			double sum = 0.0;
			double* outputPtr = output2d.data();
			for(int y=0; y!=int(n); ++y)
			{
				double dy = y - 0.5*(n-1);
				double dydy = dy * dy;
				for(int x=0; x!=int(n) ;++x)
				{
					double dx = x - 0.5*(n-1);
					double r = sqrt(dx*dx + dydy);
					*outputPtr = hannWindowFunction(r, n) * shapeFunction(r / scaleSizeInPixels);
					sum += *outputPtr;
					++outputPtr;
				}
			}
			double normFactor = 1.0 / sum;
			for(aocommon::UVector<double>::iterator i=output2d.begin(); i!=output2d.end(); ++i)
				*i *= normFactor;
		}
	}
	
	static double hannWindowFunction(double x, size_t n)
	{
		return (x*2 <= n+1) ? (0.5 * (1.0 + cos(2.0*M_PI*x / double(n+1)))) : 0.0;
	}
	
	static double shapeFunction(double x)
	{
		return (x < 1.0) ? (1.0 - x*x) : 0.0;
	}
};

#endif
