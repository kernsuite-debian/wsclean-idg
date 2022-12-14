#include "commandline.h"
#include "wsclean.h"

#include <wscversion.h>

#include "../structures/numberlist.h"

#include <aocommon/fits/fitswriter.h>
#include <aocommon/logger.h>
#include <aocommon/radeccoord.h>
#include <aocommon/units/angle.h>
#include <aocommon/units/fluxdensity.h>

#include <schaapcommon/fitters/spectralfitter.h>
#include <schaapcommon/h5parm/jonesparameters.h>

#include <boost/algorithm/string.hpp>

#include <iostream>
#include <optional>
#include <string>
#include <sstream>

using aocommon::Logger;
using aocommon::units::Angle;
using aocommon::units::FluxDensity;

void CommandLine::printHelp() {
  std::cout
      << "Syntax: wsclean [options] <input-ms> [<2nd-ms> [..]]\n"
         "Will create cleaned images of the input ms(es).\n"
         "If multiple mses are specified, they need to be phase-rotated to the "
         "same point on the sky.\n\n"
         "Options can be:\n\n"
         "  ** GENERAL OPTIONS **\n"
         "-version\n"
         "   Print WSClean's version and exit.\n"
         "-j <threads>\n"
         "   Specify number of computing threads to use, i.e., number of cpu "
         "cores that will be used.\n"
         "   Default: use all cpu cores.\n"
         "-parallel-gridding <n>\n"
         "   Will execute multiple gridders simultaneously. This can make "
         "things faster in certain cases,\n"
         "   but will increase memory usage. \n"
         "-parallel-reordering <n>\n"
         "   Process the reordering with multipliple threads. \n"
         "-no-work-on-master\n"
         "   In MPI runs, do not use the master for gridding. This may be "
         "useful if the\n"
         "   resources such as memory of the master are limited.\n"
         "-mem <percentage>\n"
         "   Limit memory usage to the given fraction of the total system "
         "memory. This is an approximate value.\n"
         "   Default: 100.\n"
         "-abs-mem <memory limit>\n"
         "   Like -mem, but this specifies a fixed amount of memory in "
         "gigabytes.\n"
         "-verbose (or -v)\n"
         "   Increase verbosity of output.\n"
         "-log-time\n"
         "   Add date and time to each line in the output.\n"
         "-quiet\n"
         "   Do not output anything but errors.\n"
         "-reorder\n"
         "-no-reorder\n"
         "   Force or disable reordering of Measurement Set. This can be "
         "faster when the measurement set needs to\n"
         "   be iterated several times, such as with many major iterations or "
         "in channel imaging mode.\n"
         "   Default: only reorder when in channel imaging mode.\n"
         "-temp-dir <directory>\n"
         "   Set the temporary directory used when reordering files. Default: "
         "same directory as input measurement set.\n"
         "-update-model-required (default), and\n"
         "-no-update-model-required\n"
         "   These two options specify whether the model data column is "
         "required to\n"
         "   contain valid model data after imaging. It can save time to not "
         "update\n"
         "   the model data column.\n"
         "-no-dirty\n"
         "   Do not save the dirty image.\n"
         "-save-first-residual\n"
         "   Save the residual after the first iteration.\n"
         "-save-weights\n"
         "   Save the gridded weights in the a fits file named "
         "<image-prefix>-weights.fits.\n"
         "-save-uv\n"
         "   Save the gridded uv plane, i.e., the FFT of the residual image. "
         "The UV plane is complex, hence\n"
         "   two images will be output: <prefix>-uv-real.fits and "
         "<prefix>-uv-imag.fits.\n"
         "-reuse-psf <prefix>\n"
         "   Load the psf(s) from the given prefix and skip the inversion for "
         "the psf image.\n"
         "-reuse-dirty <prefix>\n"
         "   Load the dirty from the given prefix and skip the inversion for "
         "the dirty image.\n"
         "-apply-primary-beam\n"
         "   Calculate and apply the primary beam and save images for the "
         "Jones components, with weighting identical to the\n"
         "   weighting as used by the imager. Only available for instruments\n"
         "   supported by EveryBeam.\n"
         "-reuse-primary-beam\n"
         "   If a primary beam image exists on disk, reuse those images.\n"
         "-use-differential-lofar-beam\n"
         "   Assume the visibilities have already been beam-corrected for the "
         "reference direction.\n"
         "   By default, WSClean will use the information in the measurement "
         "set to determine\n"
         "   if the differential beam should be applied for obtaining proper "
         "flux levels.\n"
         "-primary-beam-limit <limit>\n"
         "   Level at which to trim the beam when performing image-based beam\n"
         "   correction,. Default: 0.005.\n"
         "-mwa-path <path>\n"
         "   Set path where to find the MWA beam file(s).\n"
         "-save-psf-pb\n"
         "   When applying beam correction, also save the primary-beam "
         "corrected PSF image.\n"
         "-pb-grid-size <npixel>\n"
         "   Specify the grid size in number of pixels at which to evaluate "
         "   the primary beam.\n"
         "   Typically, the primary beam is calculated at a coarse resolution "
         "grid \n"
         "   and interpolated, to reduce the time spent in evaluating the "
         "beam. \n"
         "   This parameter controls the resolution of the grid at which to "
         "evaluate \n"
         "   the primary beam. For rectangular images, pb-grid-size \n"
         "   indicates the number of pixels along the shortest dimension. \n"
         "   The total number of pixels in the primary beam grid thus amounts "
         "to: \n\n"
         "   max(width, height) / min(width, height) * pb-grid-size**2. \n\n"
         "   Default: 32.\n"
         "-beam-model\n"
         "   Specify the beam model, only relevant for SKA and LOFAR. "
         "Available "
         "models are Hamaker, Lobes, OskarDipole, OskarSphericalWave.\n"
         "   Input is case insensitive. Default is Hamaker for LOFAR and\n"
         "   OskarSphericalWave for SKA.\n"
         "-beam-mode\n"
         "   [DEBUGGING ONLY] Manually specify the beam mode. Only relevant "
         "for simulated SKA measurement sets. \n"
         "   Available modes are array_factor, element and full.\n"
         "   Input is case insensitive. Default is full.\n"
         "-beam-normalisation-mode\n"
         "    [DEBUGGING ONLY] Manually specify the normalisation of the beam. "
         "Only "
         "relevant for simulated SKA measurement sets. \n"
         "    Available modes are none, preapplied, full, and amplitude. "
         "Default is preapplied.\n"
         "-dry-run\n"
         "   Parses the command line and quits afterwards. No imaging is "
         "done.\n"
         "\n"
         "  ** WEIGHTING OPTIONS **\n"
         "-weight <weightmode>\n"
         "   Weightmode can be: natural, uniform, briggs. Default: uniform. "
         "When using Briggs' weighting,\n"
         "   add the robustness parameter, like: \"-weight briggs 0.5\".\n"
         "-super-weight <factor>\n"
         "   Increase the weight gridding box size, similar to Casa's "
         "superuniform weighting scheme. Default: 1.0\n"
         "   The factor can be rational and can be less than one for subpixel "
         "weighting.\n"
         "-mf-weighting\n"
         "   In spectral mode, calculate the weights as if the image was made "
         "using MF. This makes sure that the sum of\n"
         "   channel images equals the MF weights. Otherwise, the channel "
         "image will become a bit more naturally weighted.\n"
         "   This is only relevant for weighting modes that require gridding "
         "(i.e., Uniform, Briggs').\n"
         "   Default: off, unless -join-channels is specified.\n"
         "-no-mf-weighting\n"
         "   Opposite of -ms-weighting; can be used to turn off MF weighting "
         "in -join-channels mode.\n"
         "-weighting-rank-filter <level>\n"
         "   Filter the weights and set high weights to the local mean. The "
         "level parameter specifies\n"
         "   the filter level; any value larger than level*localmean will be "
         "set to level*localmean.\n"
         "-weighting-rank-filter-size <size>\n"
         "   Set size of weighting rank filter. Default: 16.\n"
         "-taper-gaussian <beamsize>\n"
         "   Taper the weights with a Gaussian function. This will reduce the "
         "contribution of long baselines.\n"
         "   The beamsize is by default in asec, but a unit can be specified "
         "(\"2amin\").\n"
         "-taper-tukey <lambda>\n"
         "   Taper the outer weights with a Tukey transition. Lambda specifies "
         "the size of the transition; use in\n"
         "   combination with -maxuv-l.\n"
         "-taper-inner-tukey <lambda>\n"
         "   Taper the weights with a Tukey transition. Lambda specifies the "
         "size of the transition; use in\n"
         "   combination with -minuv-l.\n"
         "-taper-edge <lambda>\n"
         "   Taper the weights with a rectangle, to keep a space of lambda "
         "between the edge and gridded visibilities.\n"
         "-taper-edge-tukey <lambda>\n"
         "   Taper the edge weights with a Tukey window. Lambda is the size of "
         "the Tukey transition. When -taper-edge\n"
         "   is also specified, the Tukey transition starts inside the inner "
         "rectangle.\n"
         "-use-weights-as-taper\n"
         "   Will not use visibility weights when determining the imaging "
         "weights.\n"
         "   This has the effect that e.g. uniform weighting can be modified "
         "by increasing\n"
         "   the visibility weight of certain baselines. Without this option, "
         "uniform imaging\n"
         "   weights absorb the visibility weight to make the weighting truly "
         "uniform.\n"
         "-store-imaging-weights\n"
         "   Will store the imaging weights in a column named "
         "'IMAGING_WEIGHT_SPECTRUM'.\n"
         "\n"
         "  ** INVERSION OPTIONS **\n"
         "-name <image-prefix>\n"
         "   Use image-prefix as prefix for output files. Default is "
         "'wsclean'.\n"
         "-size <width> <height>\n"
         "   Set the output image size in number of pixels (without padding).\n"
         "-padding <factor>\n"
         "   Pad images by the given factor during inversion to avoid "
         "aliasing. Default: 1.2 (=20%).\n"
         "-scale <pixel-scale>\n"
         "   Scale of a pixel. Default unit is degrees, but can be "
         "specificied, e.g. -scale 20asec. Default: 0.01deg.\n"
         "-predict\n"
         "   Only perform a single prediction for an existing image. Doesn't "
         "do any imaging or cleaning.\n"
         "   The input images should have the same name as the model output "
         "images would have in normal imaging mode.\n"
         //		"-predict-channels <nchannels>\n"
         //		"   Interpolate from a given number of images to the
         // number of channels that are predicted\n" 		"   as specified
         // by -channelsout. Will interpolate using the frequencies of the
         // images.\n" 		"   Use one of the -fit-spectral-... options to
         // specify the interpolation method
         /// freedom.\n" 		"   Only used when -predict is
         /// specified.\n"
         "-continue\n"
         "   Will continue an earlier WSClean run. Earlier model images will "
         "be read and model visibilities will be\n"
         "   subtracted to create the first dirty residual. CS should have "
         "been used in the earlier run, and model data"
         "   should have been written to the measurement set for this to work. "
         "Default: off.\n"
         "-subtract-model\n"
         "   Subtract the model from the data column in the first iteration. "
         "This can be used to reimage\n"
         "   an already cleaned image, e.g. at a different resolution.\n"
         "-channels-out <count>\n"
         "   Splits the bandwidth and makes count nr. of images. Default: 1.\n"
         "-shift <ra> <dec>\n"
         "   Shift the phase centre to the given location. The shift is along\n"
         "   the tangential plane.\n"
         "-gap-channel-division\n"
         "   In case of irregular frequency spacing, this option can be used "
         "to not try and split channels\n"
         "   to make the output channel bandwidth similar, but instead to "
         "split largest gaps first.\n"
         "-channel-division-frequencies <list>\n"
         "   Split the bandwidth at the specified frequencies (in Hz) before "
         "the normal bandwidth\n"
         "   division is performed. This can e.g. be useful for imaging "
         "multiple bands with irregular\n"
         "   number of channels.\n"
         "-nwlayers <nwlayers>\n"
         "   Number of w-layers to use. Default: minimum suggested #w-layers "
         "for first MS.\n"
         "-nwlayers-factor <factor>\n"
         "   Use automatic calculation of the number of w-layers, but multiple "
         "that number by\n"
         "   the given factor. This can e.g. be useful for increasing "
         "w-accuracy.\n"
         "-nwlayers-for-size <width> <height>\n"
         "   Use the minimum suggested w-layers for an image of the given "
         "size. Can e.g. be used to increase\n"
         "   accuracy when predicting small part of full image. \n"
         "-no-small-inversion and -small-inversion\n"
         "   Perform inversion at the Nyquist resolution and upscale the image "
         "to the requested image size afterwards.\n"
         "   This speeds up inversion considerably, but makes aliasing "
         "slightly worse. This effect is\n"
         "   in most cases <1%. Default: on.\n"
         "-grid-mode <\"nn\", \"kb\" or \"rect\">\n"
         "   Kernel and mode used for gridding: kb = Kaiser-Bessel (default "
         "with 7 pixels), nn = nearest\n"
         "   neighbour (no kernel), more options: rect, kb-no-sinc, gaus, bn. "
         "Default: kb.\n"
         "-kernel-size <size>\n"
         "   Gridding antialiasing kernel size. Default: 7.\n"
         "-oversampling <factor>\n"
         "   Oversampling factor used during gridding. Default: 63.\n"
         "-make-psf\n"
         "   Always make the psf, even when no cleaning is performed.\n"
         "-make-psf-only\n"
         "   Only make the psf, no images are made.\n"
         "-visibility-weighting-mode [normal/squared/unit]\n"
         "   Specify visibility weighting modi. Affects how the weights "
         "(normally) stored in\n"
         "   WEIGHT_SPECTRUM column are applied. Useful for estimating e.g. "
         "EoR power spectra errors.\n"
         "   Normally one would use this in combination with "
         "-no-normalize-for-weighting.\n"
         "-no-normalize-for-weighting\n"
         "   Disable the normalization for the weights, which makes the PSF's "
         "peak one. See\n"
         "   -visibility-weighting-mode. Only useful with natural weighting.\n"
         "-baseline-averaging <size-in-wavelengths>\n"
         "   Enable baseline-dependent averaging. The specified size is in "
         "number of wavelengths (i.e., uvw-units). One way\n"
         "   to calculate this is with <baseline in nr. of lambdas> * 2pi * "
         "<acceptable integration in s> / (24*60*60).\n"
         "-simulate-noise <stddev-in-jy>\n"
         "   Will replace every visibility by a Gaussian distributed value "
         "with given standard deviation before imaging.\n"
         "-simulate-baseline-noise <filename>\n"
         "   Like -simulate-noise, but the stddevs are provided per baseline, "
         "in a text file\n"
         "   with antenna1 and antenna2 indices and the stddev per line, "
         "separated by spaces, e.g. \"0 1 3.14\".\n"
         "-direct-ft\n"
         "   Do not grid the visibilities on the uv grid, but instead perform "
         "a fully accurate direct Fourier transform (slow!).\n"
         "-use-idg\n"
         "   Use the 'image-domain gridder' (Van der Tol et al.) to do the "
         "inversions and predictions.\n"
         "-idg-mode [cpu/gpu/hybrid]\n"
         "   Sets the IDG mode. Default: cpu. Hybrid is recommended when a GPU "
         "is available.\n"
         "-use-wgridder\n"
         "   Use the w-gridding gridder developed by Martin Reinecke.\n"
         "-wgridder-accuracy <value>\n"
         "   Set the w-gridding accuracy. Default: 1e-4\n"
         "   Useful range: 1e-2 to 1e-6\n"
         "\n"
         "  ** A-TERM GRIDDING **\n"
         "-aterm-config <filename>\n"
         "   Specify a parameter set describing how a-terms should be applied. "
         "Please refer to the documentation for\n"
         "   details of the configuration file format. Applying a-terms is "
         "only possible when IDG is enabled.\n"
         "-grid-with-beam\n"
         "   Apply a-terms to correct for the primary beam. This is only "
         "possible when IDG is enabled.\n"
         "-beam-aterm-update <seconds>\n"
         "   Set the ATerm update time in seconds. The default is every 300 "
         "seconds.\n"
         "   It also sets the interval over which to calculate the primary "
         "beam when using\n"
         "   -apply-primary-beam when not gridding with the beam.\n"
         "-aterm-kernel-size <double>\n"
         "   Kernel size reserved for aterms by IDG.\n"
         "-apply-facet-solutions <path-to-file> <name1[,name2]>\n"
         "   Apply solutions from the provided (h5) file per facet "
         "when gridding facet based images.\n"
         "   Provided file is assumed to be in H5Parm format.\n"
         "   Filename is followed by a comma separated list of strings "
         "specifying which "
         "sol tabs from the provided H5Parm file are used.\n"
         "-apply-facet-beam\n"
         "   Apply beam gains to facet center when gridding "
         "facet based images\n"
         "-facet-beam-update <seconds>\n"
         "   Set the facet beam update time in seconds. The default is every "
         "120 seconds.\n"
         "-save-aterms\n"
         "   Output a fits file for every aterm update, containing the applied "
         "image for every station.\n"
         "\n"
         "  ** DATA SELECTION OPTIONS **\n"
         "-pol <list>\n"
         "   Default: \'I\'. Possible values: XX, XY, YX, YY, I, Q, U, V, RR, "
         "RL, LR or LL (case insensitive).\n"
         "   It is allowed but not necessary to separate with commas, e.g.: "
         "'xx,xy,yx,yy'."
         "   Two or four polarizations can be joinedly cleaned (see "
         "'-joinpolarizations'), but \n"
         "   this is not the default. I, Q, U and V polarizations will be "
         "directly calculated from\n"
         "   the visibilities, which might require correction to get to real "
         "IQUV values. The\n"
         "   'xy' polarization will output both a real and an imaginary image, "
         "which allows calculating\n"
         "   true Stokes polarizations for those telescopes.\n"
         "-interval <start-index> <end-index>\n"
         "   Only image the given time interval. Indices specify the "
         "timesteps, end index is exclusive.\n"
         "   Default: image all time steps.\n"
         "-intervals-out <count>\n"
         "   Number of intervals to image inside the selected global interval. "
         "Default: 1\n"
         "-even-timesteps\n"
         "   Only select even timesteps. Can be used together with "
         "-odd-timesteps to determine noise values.\n"
         "-odd-timesteps\n"
         "   Only select odd timesteps.\n"
         "-channel-range <start-channel> <end-channel>\n"
         "   Only image the given channel range. Indices specify channel "
         "indices, end index is exclusive.\n"
         "   Default: image all channels.\n"
         "-field <list>\n"
         "   Image the given field id(s). A comma-separated list of field ids "
         "can be provided. When multiple\n"
         "   fields are given, all fields should have the same phase centre. "
         "Specifying '-field all' will image\n"
         "   all fields in the measurement set. Default: first field (id 0).\n"
         "-spws <list>\n"
         "   Selects only the spws given in the list. list should be a "
         "comma-separated list of integers. Default: all spws.\n"
         "-data-column <columnname>\n"
         "   Default: CORRECTED_DATA if it exists, otherwise DATA will be "
         "used.\n"
         "-maxuvw-m <meters>\n"
         "-minuvw-m <meters>\n"
         "   Set the min/max baseline distance in meters.\n"
         "-maxuv-l <lambda>\n"
         "-minuv-l <lambda>\n"
         "   Set the min/max uv distance in lambda.\n"
         "-maxw <percentage>\n"
         "   Do not grid visibilities with a w-value higher than the given "
         "percentage of the max w, to save speed.\n"
         "   Default: grid everything\n"
         "\n"
         "  ** DECONVOLUTION OPTIONS **\n"
         "-niter <niter>\n"
         "   Maximum number of clean iterations to perform. Default: 0 (=no "
         "cleaning)\n"
         "-nmiter <nmiter>\n"
         "   Maximum number of major clean (inversion/prediction) iterations. "
         "Default: 20."
         "   A value of 0 means no limit.\n"
         "-threshold <threshold>\n"
         "   Stopping clean thresholding in Jy. Default: 0.0\n"
         "-auto-threshold <sigma>\n"
         "   Estimate noise level using a robust estimator and stop at sigma x "
         "stddev.\n"
         "-auto-mask <sigma>\n"
         "   Construct a mask from found components and when a threshold of "
         "sigma is reached, continue\n"
         "   cleaning with the mask down to the normal threshold. \n"
         "-local-rms\n"
         "   Instead of using a single RMS for auto thresholding/masking, use "
         "a spatially varying\n"
         "   RMS image.\n"
         "-local-rms-window\n"
         "   Size of window for creating the RMS background map, in number of "
         "PSFs. Default: 25 psfs.\n"
         "-local-rms-method\n"
         "   Either 'rms' (default, uses sliding window RMS) or 'rms-with-min' "
         "(use max(window rms, 0.3 x window min)).\n"
         "-gain <gain>\n"
         "   Cleaning gain: Ratio of peak that will be subtracted in each "
         "iteration. Default: 0.1\n"
         "-mgain <gain>\n"
         "   Cleaning gain for major iterations: Ratio of peak that will be "
         "subtracted in each major\n"
         "   iteration. To use major iterations, 0.85 is a good value. "
         "Default: 1.0\n"
         "-join-polarizations\n"
         "   Perform deconvolution by searching for peaks in the sum of "
         "squares of the polarizations,\n"
         "   but subtract components from the individual images. Only possible "
         "when imaging two or four Stokes\n"
         "   or linear parameters. Default: off.\n"
         "-link-polarizations <pollist>\n"
         "   Links all polarizations to be cleaned from the given list: "
         "components are found in the\n"
         "   given list, but cleaned from all polarizations. \n"
         "-facet-regions <facets.reg>\n"
         "   Split the image into facets using the facet regions defined in "
         " the facets.reg file. Default: off.\n"
         "-join-channels\n"
         "   Perform deconvolution by searching for peaks in the MF image,\n"
         "but subtract components from individual channels.\n"
         "   This will turn on mf-weighting by default. Default: off.\n"
         "-spectral-correction <reffreq> <term list>\n"
         "   Enable correction of the given spectral function inside "
         "deconvolution.\n"
         "   This can e.g. avoid downweighting higher frequencies because of\n"
         "   reduced flux density. 1st term is total flux, 2nd is si, 3rd "
         "curvature, etc. \n"
         "   Example: -spectral-correction 150e6 83.084,-0.699,-0.110\n"
         "-no-fast-subminor\n"
         "   Do not use the subminor loop optimization during (non-multiscale) "
         "cleaning. Default: use the optimization.\n"
         "-multiscale\n"
         "   Clean on different scales. This is a new algorithm. Default: "
         "off.\n"
         "   This parameter invokes the optimized multiscale algorithm "
         "published by Offringa & Smirnov (2017).\n"
         "-multiscale-scale-bias\n"
         "   Parameter to prevent cleaning small scales in the large-scale "
         "iterations. A lower\n"
         "   bias will give more focus to larger scales. Default: 0.6\n"
         "-multiscale-max-scales <n>\n"
         "   Set the maximum number of scales that WSClean should use in "
         "multiscale cleaning.\n"
         "   Only relevant when -multiscale-scales is not set. Default: "
         "unlimited.\n"
         "-multiscale-scales <comma-separated list of sizes in pixels>\n"
         "   Sets a list of scales to use in multi-scale cleaning. If unset, "
         "WSClean will select the delta\n"
         "   (zero) scale, scales starting at four times the synthesized PSF, "
         "and increase by a factor of\n"
         "   two until the maximum scale is reached or the maximum number of "
         "scales is reached.\n"
         "   Example: -multiscale-scales 0,5,12.5\n"
         "-multiscale-shape <shape>\n"
         "   Sets the shape function used during multi-scale clean. Either "
         "'tapered-quadratic' (default) or 'gaussian'.\n"
         "-multiscale-gain <gain>\n"
         "   Size of step made in the subminor loop of multi-scale. Default "
         "currently 0.2, but shows sign of instability.\n"
         "   A value of 0.1 might be more stable.\n"
         "-multiscale-convolution-padding <padding>\n"
         "   Size of zero-padding for convolutions during the multi-scale "
         "cleaning. Default: 1.1\n"
         "-no-multiscale-fast-subminor\n"
         "   Disable the 'fast subminor loop' optimization, that will only "
         "search a part of the\n"
         "   image during the multi-scale subminor loop. The optimization is "
         "on by default.\n"
         "-python-deconvolution <filename>\n"
         "   Run a custom deconvolution algorithm written in Python. See "
         "manual\n"
         "   for the interface.\n"
         "-iuwt\n"
         "   Use the IUWT deconvolution algorithm.\n"
         "-iuwt-snr-test / -no-iuwt-snr-test\n"
         "   Stop (/do not stop) IUWT when the SNR decreases. This might help "
         "limitting divergence, but can\n"
         "   occasionally also stop the algorithm too early. Default: no SNR "
         "test.\n"
         "-moresane-ext <location>\n"
         "   Use the MoreSane deconvolution algorithm, installed at the "
         "specified location.\n"
         "-moresane-arg <arguments>\n"
         "   Pass the specified arguments to moresane. Note that multiple "
         "parameters have to be\n"
         "   enclosed in quotes.\n"
         "-moresane-sl <sl1,sl2,...>\n"
         "   MoreSane --sigmalevel setting for each major loop iteration. "
         "Useful to start at high\n"
         "   levels and go down with subsequent loops, e.g. 20,10,5\n"
         "-save-source-list\n"
         "   Saves the found clean components as a BBS/DP3 text sky model. "
         "This parameter \n"
         "   enables Gaussian shapes during multi-scale cleaning "
         "(-multiscale-shape gaussian).\n"
         "-clean-border <percentage>\n"
         "   Set the border size in which no cleaning is performed, in "
         "percentage of the width/height of the image.\n"
         "   With an image size of 1000 and clean border of 1%, each border is "
         "10 pixels. Default: 0%\n"
         "-fits-mask <mask>\n"
         "   Use the specified fits-file as mask during cleaning.\n"
         "-casa-mask <mask>\n"
         "   Use the specified CASA mask as mask during cleaning.\n"
         "-horizon-mask <distance>\n"
         "   Use a mask that avoids cleaning emission beyond the horizon. "
         "Distance is an angle (e.g. \"5deg\")\n"
         "   that (when positive) decreases the size of the mask to stay "
         "further away from the horizon.\n"
         "-no-negative\n"
         "   Do not allow negative components during cleaning. Not the "
         "default.\n"
         "-negative\n"
         "   Default on: opposite of -nonegative.\n"
         "-stop-negative\n"
         "   Stop on negative components. Not the default.\n"
         "-fit-spectral-pol <nterms>\n"
         "   Fit a polynomial over frequency to each clean component. This has "
         "only effect\n"
         "   when the channels are joined with -join-channels.\n"
         "-fit-spectral-log-pol <nterms>\n"
         "   Like fit-spectral-pol, but fits a logarithmic polynomial over "
         "frequency instead.\n"
         "-force-spectrum <fitsfile>\n"
         "   Uses the fits file to force spectral indices (or other/more terms)"
         "   during the deconvolution.\n"
         "-deconvolution-channels <nchannels>\n"
         "   Decrease the number of channels as specified by -channels-out to "
         "the given number for\n"
         "   deconvolution. Only possible in combination with one of the "
         "-fit-spectral options.\n"
         "   Proper residuals/restored images will only be returned when mgain "
         "< 1.\n"
         "-squared-channel-joining\n"
         "   Use with -join-channels to perform peak finding in the sum of "
         "squared values over\n"
         "   channels, instead of the normal sum. This is useful for imaging "
         "QU polarizations\n"
         "   with non-zero rotation measures, for which the normal sum is "
         "insensitive.\n"
         "-parallel-deconvolution <maxsize>\n"
         "   Deconvolve subimages in parallel. Subimages will be at most of "
         "the given size.\n"
         "-deconvolution-threads <n>\n"
         "   Number of threads to use during deconvolution. On machines with "
         "a large nr of cores, this may be used to decrease the memory "
         "usage.\n"
         "   If not specified, the number of threads during deconvolution "
         "is controlled with the -j option.\n"
         "\n"
         "  ** RESTORATION OPTIONS **\n"
         "-restore <input residual> <input model> <output image>\n"
         "   Restore the model image onto the residual image and save it in "
         "output image. By\n"
         "   default, the beam parameters are read from the residual image. If "
         "this parameter\n"
         "   is given, wsclean will do the restoring and then exit: no "
         "cleaning is performed.\n"
         "-restore-list <input residual> <input list> <output image>\n"
         "   Restore a source list onto the residual image and save it in "
         "output image. Except\n"
         "   for the model input format, this parameter behaves equal to "
         "-restore.\n"
         "-beam-size <arcsec>\n"
         "   Set a circular beam size (FWHM) in arcsec for restoring the clean "
         "components. This is\n"
         "   the same as -beam-shape <size> <size> 0.\n"
         "-beam-shape <maj in arcsec> <min in arcsec> <position angle in deg>\n"
         "   Set the FWHM beam shape for restoring the clean components. "
         "Defaults units for maj and min are arcsec, and\n"
         "   degrees for PA. Can be overriden, e.g. '-beam-shape 1amin 1amin "
         "3deg'. Default: shape of PSF.\n"
         "-fit-beam\n"
         "   Determine beam shape by fitting the PSF (default if PSF is "
         "made).\n"
         "-no-fit-beam\n"
         "   Do not determine beam shape from the PSF.\n"
         "-beam-fitting-size <factor>\n"
         "   Use a fitting box the size of <factor> times the theoretical beam "
         "size for fitting a Gaussian to the PSF.\n"
         "-theoretic-beam\n"
         "   Write the beam in output fits files as calculated from the "
         "longest projected baseline.\n"
         "   This method results in slightly less accurate beam "
         "size/integrated fluxes, but provides a beam size\n"
         "   without making the PSF for quick imaging. Default: off.\n"
         "-circular-beam\n"
         "   Force the beam to be circular: bmin will be set to bmaj.\n"
         "-elliptical-beam\n"
         "   Allow the beam to be elliptical. Default.\n"
         "\n"
         "For detailed help, check the WSClean website: "
         "https://wsclean.readthedocs.io/ .\n";
}

void CommandLine::printHeader() {
  Logger::Info << "\n"
                  "WSClean version " WSCLEAN_VERSION_STR
                  " (" WSCLEAN_VERSION_DATE
                  ")\n"
                  "This software package is released under the GPL version 3.\n"
                  "Author: Andr?? Offringa (offringa@gmail.com).\n\n";
#ifndef NDEBUG
  Logger::Info
      << "\n"
         "WARNING: Symbol NDEBUG was not defined; this WSClean version was\n"
         "compiled as a DEBUG version. This can seriously affect "
         "performance!\n\n";
#endif
}

size_t CommandLine::parse_size_t(const char* param, const char* name) {
  char* endptr;
  errno = 0;
  long v = strtol(param, &endptr, 0);
  if (*endptr != 0 || endptr == param || errno != 0) {
    std::ostringstream msg;
    msg << "Could not parse value '" << param << "' for parameter -" << name
        << " to an integer";
    throw std::runtime_error(msg.str());
  }
  if (v < 0) {
    std::ostringstream msg;
    msg << "Invalid value (" << v << ") for parameter -" << name;
    throw std::runtime_error(msg.str());
  }
  return v;
}

std::vector<std::string> CommandLine::parseStringList(const char* param) {
  std::vector<std::string> list;
  boost::split(list, param, [](char c) { return c == ','; });
  return list;
}

double CommandLine::parse_double(const char* param, const char* name) {
  char* endptr;
  double v = std::strtod(param, &endptr);
  if (*endptr != 0 || endptr == param || !std::isfinite(v)) {
    std::ostringstream msg;
    msg << "Could not parse value '" << param << "' for parameter -" << name
        << " to a (double-precision) floating point value";
    throw std::runtime_error(msg.str());
  }
  return v;
}

double CommandLine::parse_double(const char* param, double lowerLimit,
                                 const char* name, bool inclusive) {
  double v = parse_double(param, name);
  if (v < lowerLimit || (v <= lowerLimit && !inclusive)) {
    std::ostringstream msg;
    msg << "Parameter value for -" << name << " was " << v << " but ";
    if (inclusive)
      msg << "is not allowed to be smaller than " << lowerLimit;
    else
      msg << "has to be larger than " << lowerLimit;
    throw std::runtime_error(msg.str());
  }
  return v;
}

bool CommandLine::ParseWithoutValidation(WSClean& wsclean, int argc,
                                         const char* argv[], bool isSlave) {
  if (argc < 2) {
    if (!isSlave) {
      printHeader();
      printHelp();
    }
    return false;
  }

  Settings& settings = wsclean.GetSettings();
  int argi = 1;
  bool mfWeighting = false, noMFWeighting = false, dryRun = false;
  auto atermKernelSize = std::make_optional<double>();
  Logger::SetVerbosity(Logger::kNormalVerbosity);
  while (argi < argc && argv[argi][0] == '-') {
    const std::string param =
        argv[argi][1] == '-' ? (&argv[argi][2]) : (&argv[argi][1]);
    if (param == "version") {
      if (!isSlave) {
        printHeader();
#ifdef HAVE_EVERYBEAM
        Logger::Info << "EveryBeam is available.\n";
#endif
#ifdef HAVE_IDG
        Logger::Info << "IDG is available.\n";
#endif
        Logger::Info << "WGridder is available.\n";
      }
      return false;
    } else if (param == "help") {
      if (!isSlave) {
        printHeader();
        printHelp();
      }
      return false;
    } else if (param == "quiet") {
      Logger::SetVerbosity(Logger::kQuietVerbosity);
    } else if (param == "v" || param == "verbose") {
      Logger::SetVerbosity(Logger::kVerboseVerbosity);
    } else if (param == "log-time") {
      Logger::SetLogTime(true);
    } else if (param == "temp-dir" || param == "tempdir") {
      ++argi;
      settings.temporaryDirectory = argv[argi];
      if (param == "tempdir") deprecated(isSlave, param, "temp-dir");
    } else if (param == "save-weights" || param == "saveweights") {
      settings.isWeightImageSaved = true;
      if (param == "saveweights") deprecated(isSlave, param, "save-weights");
    } else if (param == "save-uv" || param == "saveuv") {
      settings.isUVImageSaved = true;
      if (param == "saveuv") deprecated(isSlave, param, "save-uv");
    } else if (param == "reuse-psf") {
      ++argi;
      settings.reusePsf = true;
      settings.reusePsfPrefix = argv[argi];
    } else if (param == "reuse-dirty") {
      ++argi;
      settings.reuseDirty = true;
      settings.reuseDirtyPrefix = argv[argi];
    } else if (param == "predict") {
      settings.mode = Settings::PredictMode;
    } else if (param == "predict-channels") {
      ++argi;
      settings.predictionChannels =
          parse_size_t(argv[argi], "predict-channels");
    } else if (param == "continue") {
      settings.continuedRun = true;
      // Always make a PSF -- otherwise no beam size is available for
      // restoring the existing model.
      settings.makePSF = true;
    } else if (param == "subtract-model") {
      settings.subtractModel = true;
    } else if (param == "size") {
      size_t width = parse_size_t(argv[argi + 1], "size"),
             height = parse_size_t(argv[argi + 2], "size");
      settings.trimmedImageWidth = width;
      settings.trimmedImageHeight = height;
      argi += 2;
    } else if (param == "padding") {
      ++argi;
      settings.imagePadding = parse_double(argv[argi], 1.0, "padding");
    } else if (param == "scale") {
      ++argi;
      settings.pixelScaleX =
          Angle::Parse(argv[argi], "scale parameter", Angle::kDegrees);
      settings.pixelScaleY = settings.pixelScaleX;
    } else if (param == "nwlayers") {
      ++argi;
      settings.nWLayers = parse_size_t(argv[argi], "nwlayers");
    } else if (param == "nwlayers-factor") {
      ++argi;
      settings.nWLayersFactor =
          parse_double(argv[argi], 0.0, "nwlayers-factor", false);
    } else if (param == "nwlayers-for-size") {
      settings.widthForNWCalculation =
          parse_size_t(argv[argi + 1], "nwlayers-for-size");
      settings.heightForNWCalculation =
          parse_size_t(argv[argi + 2], "nwlayers-for-size");
      argi += 2;
    } else if (param == "gain") {
      ++argi;
      settings.deconvolutionGain = parse_double(argv[argi], 0.0, "gain", false);
    } else if (param == "mgain") {
      ++argi;
      settings.deconvolutionMGain = parse_double(argv[argi], 0.0, "mgain");
    } else if (param == "niter") {
      ++argi;
      settings.deconvolutionIterationCount = parse_size_t(argv[argi], "niter");
    } else if (param == "nmiter") {
      ++argi;
      settings.majorIterationCount = parse_size_t(argv[argi], "nmiter");
    } else if (param == "threshold") {
      ++argi;
      settings.deconvolutionThreshold = FluxDensity::Parse(
          argv[argi], "threshold parameter", FluxDensity::kJansky);
    } else if (param == "auto-threshold") {
      ++argi;
      settings.autoDeconvolutionThreshold = true;
      settings.autoDeconvolutionThresholdSigma =
          parse_double(argv[argi], 0.0, "auto-threshold");
    } else if (param == "auto-mask") {
      ++argi;
      settings.autoMask = true;
      settings.autoMaskSigma = parse_double(argv[argi], 0.0, "auto-mask");
    } else if (param == "local-rms" || param == "rms-background") {
      settings.localRMSMethod = LocalRmsMethod::kRmsWindow;
      if (param == "rms-background") deprecated(isSlave, param, "local-rms");
    } else if (param == "local-rms-window" ||
               param == "rms-background-window") {
      ++argi;
      settings.localRMSMethod = LocalRmsMethod::kRmsWindow;
      settings.localRMSWindow =
          parse_double(argv[argi], 0.0, "local-rms-window", false);
      if (param == "rms-background-window")
        deprecated(isSlave, param, "local-rms-window");
    } else if (param == "local-rms-image" || param == "rms-background-image") {
      ++argi;
      settings.localRMSMethod = LocalRmsMethod::kRmsWindow;
      settings.localRMSImage = argv[argi];
      if (param == "rms-background-image")
        deprecated(isSlave, param, "local-rms-image");
    } else if (param == "local-rms-method" ||
               param == "rms-background-method") {
      ++argi;
      std::string method = argv[argi];
      if (method == "rms")
        settings.localRMSMethod = LocalRmsMethod::kRmsWindow;
      else if (method == "rms-with-min")
        settings.localRMSMethod = LocalRmsMethod::kRmsAndMinimumWindow;
      else
        throw std::runtime_error("Unknown RMS background method specified");
      if (param == "rms-background-method")
        deprecated(isSlave, param, "local-rms-method");
    } else if (param == "data-column" || param == "datacolumn") {
      ++argi;
      settings.dataColumnName = argv[argi];
      if (param == "datacolumn") deprecated(isSlave, param, "data-column");
    } else if (param == "pol") {
      ++argi;
      settings.polarizations = aocommon::Polarization::ParseList(argv[argi]);
    } else if (param == "beam-model") {
      ++argi;
      std::string beamModel = argv[argi];
      boost::to_upper(beamModel);
      if (beamModel == "HAMAKER" || beamModel == "LOBES" ||
          beamModel == "OSKARDIPOLE" || beamModel == "OSKARSPHERICALWAVE") {
        settings.beamModel = beamModel;
      } else {
        throw std::runtime_error(
            "Invalid beam-model: should be either Hamaker, Lobes, OskarDipole "
            "or OskarSphericalWave (case insensitive)");
      }
    } else if (param == "beam-mode") {
      ++argi;
      std::string beamMode = argv[argi];
      boost::to_upper(beamMode);
      if (beamMode == "ARRAY_FACTOR" || beamMode == "ELEMENT" ||
          beamMode == "FULL") {
        settings.beamMode = beamMode;
      } else {
        throw std::runtime_error(
            "Invalid beam-mode: should be either array_factor, element or full "
            "(case insensitive)");
      }
    } else if (param == "beam-normalisation-mode") {
      ++argi;
      std::string beamNormalisationMode = argv[argi];
      boost::to_upper(beamNormalisationMode);
      if (beamNormalisationMode == "NONE" ||
          beamNormalisationMode == "PREAPPLIED" ||
          beamNormalisationMode == "AMPLITUDE" ||
          beamNormalisationMode == "FULL") {
        settings.beamNormalisationMode = beamNormalisationMode;
      } else {
        throw std::runtime_error(
            "Invalid beam-normalisation-mode: should be either none, "
            "preapplied, amplitude or full "
            "(case insensitive)");
      }
    } else if (param == "apply-primary-beam") {
      settings.applyPrimaryBeam = true;
    } else if (param == "reuse-primary-beam") {
      settings.reusePrimaryBeam = true;
    } else if (param == "use-differential-lofar-beam") {
      // pre_applied_or_full is the beam normalisation mode
      // that implements the behaviour of
      // the old use_differential_beam option of EveryBeam
      settings.beamNormalisationMode = "preapplied_or_full";
    } else if (param == "primary-beam-limit") {
      ++argi;
      settings.primaryBeamLimit =
          parse_double(argv[argi], 0.0, "primary-beam-limit");
    } else if (param == "mwa-path") {
      ++argi;
      settings.mwaPath = argv[argi];
    } else if (param == "dry-run") {
      dryRun = true;
    } else if (param == "save-psf-pb") {
      settings.savePsfPb = true;
    } else if (param == "pb-grid-size") {
      ++argi;
      settings.primaryBeamGridSize = parse_size_t(argv[argi], "pb-grid-size");
    } else if (param == "negative") {
      settings.allowNegativeComponents = true;
    } else if (param == "no-negative" || param == "nonegative") {
      settings.allowNegativeComponents = false;
      if (param == "nonegative") deprecated(isSlave, param, "no-negative");
    } else if (param == "stop-negative" || param == "stopnegative") {
      settings.stopOnNegativeComponents = true;
      if (param == "stopnegative") deprecated(isSlave, param, "stop-negative");
    } else if (param == "python-deconvolution") {
      ++argi;
      settings.pythonDeconvolutionFilename = argv[argi];
      settings.deconvolutionIterationCount =
          std::max(size_t{1}, settings.deconvolutionIterationCount);
    } else if (param == "iuwt") {
      settings.useIUWTDeconvolution = true;
      // Currently (WSClean 1.9, 2015-08-19) IUWT deconvolution
      // seems not to work when allowing negative components. The algorithm
      // becomes unstable. Hence, turn negative components off.
      settings.allowNegativeComponents = false;
    } else if (param == "iuwt-snr-test") {
      settings.iuwtSNRTest = true;
    } else if (param == "no-iuwt-snr-test") {
      settings.iuwtSNRTest = false;
    } else if (param == "moresane-ext") {
      ++argi;
      settings.useMoreSaneDeconvolution = true;
      settings.moreSaneLocation = argv[argi];
    } else if (param == "moresane-arg") {
      ++argi;
      settings.moreSaneArgs = argv[argi];
    } else if (param == "moresane-sl") {
      ++argi;
      settings.moreSaneSigmaLevels = NumberList::ParseDoubleList(argv[argi]);
    } else if (param == "make-psf") {
      settings.makePSF = true;
    } else if (param == "make-psf-only") {
      settings.makePSFOnly = true;
    } else if (param == "name") {
      ++argi;
      settings.prefixName = argv[argi];
    } else if (param == "grid-mode" || param == "gridmode") {
      ++argi;
      std::string gridModeStr = argv[argi];
      boost::to_lower(gridModeStr);
      if (gridModeStr == "kb" || gridModeStr == "kaiserbessel" ||
          gridModeStr == "kaiser-bessel")
        settings.gridMode = GridMode::KaiserBesselKernel;
      else if (gridModeStr == "bn")
        settings.gridMode = GridMode::BlackmanNuttallKernel;
      else if (gridModeStr == "bh")
        settings.gridMode = GridMode::BlackmanHarrisKernel;
      else if (gridModeStr == "gaus")
        settings.gridMode = GridMode::GaussianKernel;
      else if (gridModeStr == "rect")
        settings.gridMode = GridMode::RectangularKernel;
      else if (gridModeStr == "kb-no-sinc")
        settings.gridMode = GridMode::KaiserBesselWithoutSinc;
      else if (gridModeStr == "nn" || gridModeStr == "nearestneighbour")
        settings.gridMode = GridMode::NearestNeighbourGridding;
      else
        throw std::runtime_error(
            "Invalid gridding mode: should be either kb (Kaiser-Bessel), nn "
            "(NearestNeighbour), bn, bh, gaus, kb-no-sinc or rect");
      if (param == "gridmode") deprecated(isSlave, param, "grid-mode");
    } else if (param == "small-inversion" || param == "smallinversion") {
      settings.smallInversion = true;
      if (param == "smallinversion")
        deprecated(isSlave, param, "small-inversion");
    } else if (param == "no-small-inversion" || param == "nosmallinversion") {
      settings.smallInversion = false;
      if (param == "nosmallinversion")
        deprecated(isSlave, param, "no-small-inversion");
    } else if (param == "interval") {
      settings.startTimestep = parse_size_t(argv[argi + 1], "interval");
      settings.endTimestep = parse_size_t(argv[argi + 2], "interval");
      argi += 2;
    } else if (param == "intervals-out" || param == "intervalsout") {
      ++argi;
      settings.intervalsOut = atoi(argv[argi]);
      if (param == "intervalsout") deprecated(isSlave, param, "intervals-out");
    } else if (param == "even-timesteps") {
      settings.evenOddTimesteps = MSSelection::EvenTimesteps;
    } else if (param == "odd-timesteps") {
      settings.evenOddTimesteps = MSSelection::OddTimesteps;
    } else if (param == "channel-range" || param == "channelrange") {
      settings.startChannel = parse_size_t(argv[argi + 1], "channel-range");
      settings.endChannel = parse_size_t(argv[argi + 2], "channel-range");
      argi += 2;
      if (param == "channelrange") deprecated(isSlave, param, "channel-range");
    } else if (param == "shift") {
      settings.hasShift = true;
      settings.shiftRA = aocommon::RaDecCoord::ParseRA(argv[argi + 1]);
      settings.shiftDec = aocommon::RaDecCoord::ParseDec(argv[argi + 2]);
      argi += 2;
    } else if (param == "channelsout" || param == "channels-out") {
      ++argi;
      settings.channelsOut = parse_size_t(argv[argi], "channels-out");
      if (param == "channelsout") deprecated(isSlave, param, "channels-out");
    } else if (param == "gap-channel-division") {
      settings.divideChannelsByGaps = true;
    } else if (param == "channel-division-frequencies") {
      ++argi;
      settings.divideChannelFrequencies =
          NumberList::ParseDoubleList(argv[argi]);
    } else if (param == "facet-regions") {
      ++argi;
      settings.facetRegionFilename = argv[argi];
    } else if (param == "join-polarizations" || param == "joinpolarizations") {
      settings.joinedPolarizationDeconvolution = true;
      if (param == "joinpolarizations")
        deprecated(isSlave, param, "join-polarizations");
    } else if (param == "link-polarizations") {
      ++argi;
      settings.joinedPolarizationDeconvolution = true;
      settings.linkedPolarizations =
          aocommon::Polarization::ParseList(argv[argi]);
    } else if (param == "join-channels" || param == "joinchannels") {
      settings.joinedFrequencyDeconvolution = true;
      if (param == "joinchannels") deprecated(isSlave, param, "join-channels");
    } else if (param == "mf-weighting" || param == "mfs-weighting" ||
               param == "mfsweighting") {
      mfWeighting = true;
      // mfs was renamed to mf in wsclean 2.7
      if (param != "mf-weighting") deprecated(isSlave, param, "mf-weighting");
    } else if (param == "no-mf-weighting" || param == "no-mfs-weighting" ||
               param == "nomfsweighting") {
      noMFWeighting = true;
      // mfs was renamed to mf in wsclean 2.7
      if (param != "no-mf-weighting")
        deprecated(isSlave, param, "no-mf-weighting");
    } else if (param == "spectral-correction") {
      settings.spectralCorrectionFrequency =
          parse_double(argv[argi + 1], 0.0, "spectral-correction", false);
      aocommon::UVector<double> list =
          NumberList::ParseDoubleList(argv[argi + 2]);
      settings.spectralCorrection.assign(list.begin(), list.end());
      argi += 2;
    } else if (param == "taper-gaussian") {
      ++argi;
      double taperBeamSize =
          Angle::Parse(argv[argi], "Gaussian taper", Angle::kArcseconds);
      settings.gaussianTaperBeamSize = taperBeamSize;
    } else if (param == "taper-edge") {
      ++argi;
      settings.edgeTaperInLambda = parse_double(argv[argi], 0.0, "taper-edge");
    } else if (param == "taper-edge-tukey") {
      ++argi;
      settings.edgeTukeyTaperInLambda =
          parse_double(argv[argi], 0.0, "taper-edge-tukey");
    } else if (param == "taper-tukey") {
      ++argi;
      settings.tukeyTaperInLambda =
          parse_double(argv[argi], 0.0, "taper-tukey");
    } else if (param == "taper-inner-tukey") {
      ++argi;
      settings.tukeyInnerTaperInLambda =
          parse_double(argv[argi], 0.0, "taper-inner-tukey");
    } else if (param == "use-weights-as-taper") {
      settings.useWeightsAsTaper = true;
    } else if (param == "store-imaging-weights") {
      settings.writeImagingWeightSpectrumColumn = true;
    } else if (param == "no-fast-subminor") {
      settings.useSubMinorOptimization = false;
    } else if (param == "multiscale") {
      settings.useMultiscale = true;
    } else if (param == "multiscale-gain") {
      ++argi;
      settings.multiscaleGain =
          parse_double(argv[argi], 0.0, "multiscale-gain", false);
    } else if (param == "multiscale-scale-bias") {
      ++argi;
      settings.multiscaleDeconvolutionScaleBias =
          parse_double(argv[argi], 0.0, "multiscale-scale-bias", false);
    } else if (param == "multiscale-max-scales") {
      ++argi;
      settings.multiscaleMaxScales =
          parse_size_t(argv[argi], "multiscale-max-scales");
    } else if (param == "multiscale-scales") {
      ++argi;
      settings.multiscaleScaleList = NumberList::ParseDoubleList(argv[argi]);
    } else if (param == "multiscale-shape") {
      ++argi;
      std::string shape = argv[argi];
      if (shape == "tapered-quadratic")
        settings.multiscaleShapeFunction =
            MultiScaleTransforms::TaperedQuadraticShape;
      else if (shape == "gaussian")
        settings.multiscaleShapeFunction = MultiScaleTransforms::GaussianShape;
      else
        throw std::runtime_error("Unknown multiscale shape function given");
    } else if (param == "multiscale-convolution-padding") {
      ++argi;
      settings.multiscaleConvolutionPadding =
          parse_double(argv[argi], 1.0, "multiscale-convolution-padding");
    } else if (param == "no-multiscale-fast-subminor") {
      settings.multiscaleFastSubMinorLoop = false;
    } else if (param == "weighting-rank-filter") {
      ++argi;
      settings.rankFilterLevel =
          parse_double(argv[argi], 0.0, "weighting-rank-filter");
    } else if (param == "weighting-rank-filter-size") {
      ++argi;
      settings.rankFilterSize =
          parse_size_t(argv[argi], "weighting-rank-filter-size");
    } else if (param == "save-source-list") {
      settings.saveSourceList = true;
      settings.multiscaleShapeFunction = MultiScaleTransforms::GaussianShape;
    } else if (param == "clean-border" || param == "cleanborder") {
      ++argi;
      settings.deconvolutionBorderRatio =
          parse_double(argv[argi], 0.0, "clean-border") * 0.01;
      if (param == "cleanborder") deprecated(isSlave, param, "clean-border");
    } else if (param == "fits-mask" || param == "fitsmask") {
      ++argi;
      settings.fitsDeconvolutionMask = argv[argi];
      if (param == "fitsmask") deprecated(isSlave, param, "fits-mask");
    } else if (param == "casa-mask" || param == "casamask") {
      ++argi;
      settings.casaDeconvolutionMask = argv[argi];
      if (param == "casamask") deprecated(isSlave, param, "casa-mask");
    } else if (param == "horizon-mask") {
      ++argi;
      settings.horizonMask = true;
      settings.horizonMaskDistance =
          Angle::Parse(argv[argi], "horizon mask distance", Angle::kDegrees);
    } else if (param == "fit-spectral-pol") {
      ++argi;
      settings.spectralFittingMode =
          schaapcommon::fitters::SpectralFittingMode::Polynomial;
      settings.spectralFittingTerms =
          parse_size_t(argv[argi], "fit-spectral-pol");
    } else if (param == "fit-spectral-log-pol") {
      ++argi;
      settings.spectralFittingMode =
          schaapcommon::fitters::SpectralFittingMode::LogPolynomial;
      settings.spectralFittingTerms =
          parse_size_t(argv[argi], "fit-spectral-log-pol");
    } else if (param == "force-spectrum") {
      ++argi;
      settings.forcedSpectrumFilename = argv[argi];
    } else if (param == "deconvolution-channels") {
      ++argi;
      settings.deconvolutionChannelCount =
          parse_size_t(argv[argi], "deconvolution-channels");
    } else if (param == "squared-channel-joining") {
      settings.squaredJoins = true;
    } else if (param == "parallel-deconvolution") {
      ++argi;
      settings.parallelDeconvolutionMaxSize =
          parse_size_t(argv[argi], "parallel-deconvolution");
    } else if (param == "deconvolution-threads") {
      ++argi;
      settings.parallelDeconvolutionMaxThreads =
          parse_size_t(argv[argi], "deconvolution-threads");
    } else if (param == "field") {
      ++argi;
      if (argv[argi] == std::string("all"))
        settings.fieldIds.assign(1, MSSelection::ALL_FIELDS);
      else {
        aocommon::UVector<int> list = NumberList::ParseIntList(argv[argi]);
        settings.fieldIds.assign(list.begin(), list.end());
      }
    } else if (param == "spws") {
      ++argi;
      aocommon::UVector<int> list = NumberList::ParseIntList(argv[argi]);
      settings.spectralWindows.insert(list.begin(), list.end());
    } else if (param == "weight") {
      ++argi;
      std::string weightArg = argv[argi];
      if (weightArg == "natural")
        settings.weightMode = WeightMode(WeightMode::NaturalWeighted);
      else if (weightArg == "uniform")
        settings.weightMode = WeightMode(WeightMode::UniformWeighted);
      else if (weightArg == "briggs") {
        ++argi;
        settings.weightMode =
            WeightMode::Briggs(parse_double(argv[argi], "weight briggs"));
      } else
        throw std::runtime_error("Unknown weighting mode specified");
    } else if (param == "super-weight" || param == "superweight") {
      ++argi;
      settings.weightMode.SetSuperWeight(
          parse_double(argv[argi], 0.0, "super-weight"));
      if (param == "superweight") deprecated(isSlave, param, "super-weight");
    } else if (param == "restore" || param == "restore-list") {
      if (param == "restore")
        settings.mode = Settings::RestoreMode;
      else
        settings.mode = Settings::RestoreListMode;
      settings.restoreInput = argv[argi + 1];
      settings.restoreModel = argv[argi + 2];
      settings.restoreOutput = argv[argi + 3];
      argi += 3;
    } else if (param == "beam-size" || param == "beamsize") {
      ++argi;
      double beam = Angle::Parse(argv[argi], "beam size", Angle::kArcseconds);
      settings.manualBeamMajorSize = beam;
      settings.manualBeamMinorSize = beam;
      settings.manualBeamPA = 0.0;
      if (param == "beamsize") deprecated(isSlave, param, "beam-size");
    } else if (param == "beam-shape" || param == "beamshape") {
      double beamMaj = Angle::Parse(argv[argi + 1], "beam shape, major axis",
                                    Angle::kArcseconds);
      double beamMin = Angle::Parse(argv[argi + 2], "beam shape, minor axis",
                                    Angle::kArcseconds);
      double beamPA = Angle::Parse(argv[argi + 3], "beam shape, position angle",
                                   Angle::kDegrees);
      argi += 3;
      settings.manualBeamMajorSize = beamMaj;
      settings.manualBeamMinorSize = beamMin;
      settings.manualBeamPA = beamPA;
      if (param == "beamshape") deprecated(isSlave, param, "beam-shape");
    } else if (param == "fit-beam" || param == "fitbeam") {
      settings.fittedBeam = true;
      if (param == "fitbeam") deprecated(isSlave, param, "fit-beam");
    } else if (param == "no-fit-beam" || param == "nofitbeam") {
      settings.fittedBeam = false;
      if (param == "nofitbeam") deprecated(isSlave, param, "no-fit-beam");
    } else if (param == "beam-fitting-size") {
      ++argi;
      settings.beamFittingBoxSize =
          parse_double(argv[argi], 0.0, "beam-fitting-size", false);
    } else if (param == "theoretic-beam" || param == "theoreticbeam") {
      settings.theoreticBeam = true;
      settings.fittedBeam = false;
      if (param == "theoreticbeam")
        deprecated(isSlave, param, "theoretic-beam");
    } else if (param == "circular-beam" || param == "circularbeam") {
      settings.circularBeam = true;
      if (param == "circularbeam") deprecated(isSlave, param, "circular-beam");
    } else if (param == "elliptical-beam" || param == "ellipticalbeam") {
      settings.circularBeam = false;
      if (param == "ellipticalbeam")
        deprecated(isSlave, param, "elliptical-beam");
    } else if (param == "kernel-size" || param == "gkernelsize") {
      ++argi;
      settings.antialiasingKernelSize = parse_size_t(argv[argi], "kernel-size");
      if (param == "gkernelsize") deprecated(isSlave, param, "kernel-size");
    } else if (param == "oversampling") {
      ++argi;
      settings.overSamplingFactor = parse_size_t(argv[argi], "oversampling");
    } else if (param == "reorder") {
      settings.forceReorder = true;
      settings.forceNoReorder = false;
    } else if (param == "no-reorder") {
      settings.forceNoReorder = true;
      settings.forceReorder = false;
    } else if (param == "update-model-required") {
      settings.modelUpdateRequired = true;
    } else if (param == "no-update-model-required") {
      settings.modelUpdateRequired = false;
    } else if (param == "j") {
      ++argi;
      settings.threadCount = parse_size_t(argv[argi], "j");
    } else if (param == "parallel-reordering") {
      ++argi;
      settings.parallelReordering =
          parse_size_t(argv[argi], "parallel-reordering");
    } else if (param == "parallel-gridding") {
      ++argi;
      settings.parallelGridding = parse_size_t(argv[argi], "parallel-gridding");
    } else if (param == "no-work-on-master") {
      settings.masterDoesWork = false;
    } else if (param == "mem") {
      ++argi;
      settings.memFraction =
          parse_double(argv[argi], 0.0, "mem", false) / 100.0;
    } else if (param == "abs-mem" || param == "absmem") {
      ++argi;
      settings.absMemLimit = parse_double(argv[argi], 0.0, "abs-mem", false);
      if (param == "absmem") deprecated(isSlave, param, "abs-mem");
    } else if (param == "maxuvw-m") {
      ++argi;
      settings.maxUVWInMeters =
          parse_double(argv[argi], 0.0, "maxuvw-m", false);
    } else if (param == "minuvw-m") {
      ++argi;
      settings.minUVWInMeters = parse_double(argv[argi], 0.0, "minuvw-m");
    } else if (param == "maxuv-l") {
      ++argi;
      settings.maxUVInLambda = parse_double(argv[argi], 0.0, "maxuv-l", false);
    } else if (param == "minuv-l") {
      ++argi;
      settings.minUVInLambda = parse_double(argv[argi], 0.0, "minuv-l");
    } else if (param == "maxw") {
      // This was to test the optimization suggested in Tasse et al., 2013,
      // Appendix C.
      ++argi;
      settings.wLimit = parse_double(argv[argi], 0.0, "maxw");
    } else if (param == "baseline-averaging") {
      ++argi;
      settings.baselineDependentAveragingInWavelengths =
          parse_double(argv[argi], 0.0, "baseline-averaging", false);
    } else if (param == "simulate-noise") {
      ++argi;
      settings.simulateNoise = true;
      settings.simulatedNoiseStdDev =
          parse_double(argv[argi], 0.0, "simulate-noise");
    } else if (param == "simulate-baseline-noise") {
      ++argi;
      settings.simulateNoise = true;
      settings.simulatedBaselineNoiseFilename = argv[argi];
    } else if (param == "aterm-config") {
      ++argi;
      settings.atermConfigFilename = argv[argi];
    } else if (param == "grid-with-beam") {
      settings.gridWithBeam = true;
    } else if (param == "beam-aterm-update") {
      ++argi;
      double val = parse_double(argv[argi], 0.0, "beam-aterm-update");
      settings.beamAtermUpdateTime = val;
      settings.primaryBeamUpdateTime = std::max<size_t>(val, 1.0);
    } else if (param == "aterm-kernel-size") {
      ++argi;
      atermKernelSize = parse_double(argv[argi], 0.0, "aterm-kernel-size");
    } else if (param == "apply-facet-solutions") {
      ++argi;
      settings.facetSolutionFiles = parseStringList(argv[argi]);
      ++argi;
      settings.facetSolutionTables = parseStringList(argv[argi]);
      if (settings.facetSolutionTables.size() > 2) {
        throw std::runtime_error(
            "List of solution tables (soltabs) should contain at most two "
            "entries.");
      }
    } else if (param == "apply-facet-beam") {
      settings.applyFacetBeam = true;
    } else if (param == "facet-beam-update") {
      ++argi;
      settings.facetBeamUpdateTime =
          parse_double(argv[argi], 0.0, "facet-beam-update");
    } else if (param == "save-aterms") {
      settings.saveATerms = true;
    } else if (param == "visibility-weighting-mode") {
      ++argi;
      std::string modeStr = argv[argi];
      boost::to_lower(modeStr);
      if (modeStr == "normal")
        settings.visibilityWeightingMode =
            VisibilityWeightingMode::NormalVisibilityWeighting;
      else if (modeStr == "squared")
        settings.visibilityWeightingMode =
            VisibilityWeightingMode::SquaredVisibilityWeighting;
      else if (modeStr == "unit")
        settings.visibilityWeightingMode =
            VisibilityWeightingMode::UnitVisibilityWeighting;
      else
        throw std::runtime_error("Unknown weighting mode: " + modeStr);
    } else if (param == "direct-ft") {
      settings.directFT = true;
      settings.imagePadding = 1.0;
      settings.smallInversion = false;
    } else if (param == "direct-ft-precision") {
      ++argi;
      std::string precStr = argv[argi];
      if (precStr == "float")
        settings.directFTPrecision = DirectFTPrecision::Float;
      else if (precStr == "double")
        settings.directFTPrecision = DirectFTPrecision::Double;
      else if (precStr == "ldouble")
        settings.directFTPrecision = DirectFTPrecision::LongDouble;
      else
        throw std::runtime_error(
            "Invalid direct ft precision specified. Allowed options: float, "
            "double and ldouble.");
    } else if (param == "use-idg") {
#if !defined(HAVE_IDG)
      throw std::runtime_error(
          "WSClean was not compiled with IDG: to use it, install IDG and "
          "recompile WSClean");
#endif
      settings.useIDG = true;
      settings.smallInversion = false;
    } else if (param == "idg-mode") {
      ++argi;
      std::string mode =
          boost::algorithm::to_lower_copy(std::string(argv[argi]));
      if (mode == "cpu")
        settings.idgMode = Settings::IDG_CPU;
      else if (mode == "gpu")
        settings.idgMode = Settings::IDG_GPU;
      else if (mode == "hybrid")
        settings.idgMode = Settings::IDG_HYBRID;
      else
        throw std::runtime_error("Unknown IDG mode: " + mode);
    } else if (param == "use-wgridder") {
      settings.useWGridder = true;
    } else if (param == "wgridder-accuracy") {
      ++argi;
      settings.wgridderAccuracy =
          parse_double(argv[argi], 0.0, "wgridder-accuracy", false);
    } else if (param == "no-dirty") {
      settings.isDirtySaved = false;
    } else if (param == "save-first-residual") {
      settings.isFirstResidualSaved = true;
    } else {
      throw std::runtime_error("Unknown parameter: " + param);
    }

    ++argi;
  }

  if (argi == argc && settings.mode != Settings::RestoreMode &&
      settings.mode != Settings::RestoreListMode)
    throw std::runtime_error("No input measurement sets given.");

  // Done parsing.

  // We print the header only now, because the logger has now been set up
  // and possibly set to quiet.
  if (!isSlave) printHeader();

  const size_t defaultAtermSize = settings.atermConfigFilename.empty() ? 5 : 16;
  settings.atermKernelSize = atermKernelSize.value_or(defaultAtermSize);

  settings.mfWeighting =
      (settings.joinedFrequencyDeconvolution && !noMFWeighting) || mfWeighting;

  // Joined polarizations is implemented by linking all polarizations
  if (settings.joinedPolarizationDeconvolution &&
      settings.linkedPolarizations.empty()) {
    settings.linkedPolarizations = settings.polarizations;
  }

  for (int i = argi; i != argc; ++i) settings.filenames.push_back(argv[i]);

  std::ostringstream commandLineStr;
  commandLineStr << "wsclean";
  for (int i = 1; i != argc; ++i) commandLineStr << ' ' << argv[i];
  wsclean.SetCommandLine(commandLineStr.str());

  return !dryRun;
}

void CommandLine::Validate(WSClean& wsclean) {
  wsclean.GetSettings().Validate();
  wsclean.GetSettings().Propagate();
}

void CommandLine::Run(class WSClean& wsclean) {
  const Settings& settings = wsclean.GetSettings();
  switch (settings.mode) {
    case Settings::RestoreMode:
      WSCFitsWriter::Restore(settings);
      break;
    case Settings::RestoreListMode:
      WSCFitsWriter::RestoreList(settings);
      break;
    case Settings::PredictMode:
      wsclean.RunPredict();
      break;
    case Settings::ImagingMode:
      wsclean.RunClean();
      break;
  }
}

void CommandLine::deprecated(bool isSlave, const std::string& param,
                             const std::string& replacement) {
  if (!isSlave)
    Logger::Warn << "!!! WARNING: Parameter \'-" << param
                 << "\' is deprecated and will be removed in a future version "
                    "of WSClean.\n"
                 << "!!!          Use parameter \'-" << replacement
                 << "\' instead.\n";
}
