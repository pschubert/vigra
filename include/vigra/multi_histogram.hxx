/************************************************************************/
/*                                                                      */
/*         Copyright 2002-2003 by Ullrich Koethe, Hans Meine            */
/*                                                                      */
/*    This file is part of the VIGRA computer vision library.           */
/*    The VIGRA Website is                                              */
/*        http://hci.iwr.uni-heidelberg.de/vigra/                       */
/*    Please direct questions, bug reports, and contributions to        */
/*        ullrich.koethe@iwr.uni-heidelberg.de    or                    */
/*        vigra@informatik.uni-hamburg.de                               */
/*                                                                      */
/*    Permission is hereby granted, free of charge, to any person       */
/*    obtaining a copy of this software and associated documentation    */
/*    files (the "Software"), to deal in the Software without           */
/*    restriction, including without limitation the rights to use,      */
/*    copy, modify, merge, publish, distribute, sublicense, and/or      */
/*    sell copies of the Software, and to permit persons to whom the    */
/*    Software is furnished to do so, subject to the following          */
/*    conditions:                                                       */
/*                                                                      */
/*    The above copyright notice and this permission notice shall be    */
/*    included in all copies or substantial portions of the             */
/*    Software.                                                         */
/*                                                                      */
/*    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND    */
/*    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES   */
/*    OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND          */
/*    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT       */
/*    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,      */
/*    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING      */
/*    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR     */
/*    OTHER DEALINGS IN THE SOFTWARE.                                   */                
/*                                                                      */
/************************************************************************/

#ifndef VIGRA_MULTI_HISTOGRAMM_HXX
#define VIGRA_MULTI_HISTOGRAMM_HXX

#include "multi_array.hxx"
#include "tinyvector.hxx"
#include "multi_gridgraph.hxx"
#include "multi_convolution.hxx"


namespace vigra{


    template< unsigned int DIM , class T_DATA ,unsigned int CHANNELS, class T_HIST >
    void multi_gaussian_histogram(
        const MultiArrayView<DIM, TinyVector<T_DATA,CHANNELS> > & image,
        const TinyVector<T_DATA,CHANNELS> minVals,
        const TinyVector<T_DATA,CHANNELS> maxVals,
        const size_t bins,
        const float sigma,
        const float sigmaBin,
        MultiArrayView<DIM+2 , T_HIST>    histogram
    ){
        typedef vigra::GridGraph< DIM , boost::undirected_tag> Graph;
        typedef typename Graph::NodeIt graph_scanner;
        typedef typename Graph::Node   Node;
        typedef T_HIST ValueType;
        typedef TinyVector<ValueType,CHANNELS> ChannelsVals;
        typedef typename MultiArrayView<DIM+2 , T_HIST>::difference_type HistCoord;
        const Graph g(image.shape());
        const ChannelsVals nBins(bins);
        histogram.init(1.0);
        // iterate over all nodes (i.e. pixels)
        for (graph_scanner n(g); n != lemon::INVALID; ++n){
            const Node node(*n);
            ChannelsVals  binIndex = image[node];
            binIndex -=minVals;
            binIndex /=maxVals;
            binIndex *=nBins;
            HistCoord histCoord;
            for(size_t d=0;d<DIM;++d){
                histCoord[d]=node[d];
            }
            for(size_t c=0;c<CHANNELS;++c){
                const float fi = binIndex[c];
                const size_t bi = std::floor(fi+0.5);
                histCoord[DIM]=std::min(bi,static_cast<size_t>(bins-1));
                histCoord[DIM+1]=c;
                histogram[histCoord]+=1.0;
            }
        }

        MultiArray<DIM+2 , T_HIST>    histogramBuffer(histogram);
        Kernel1D<float> gauss,gaussBin;
        gauss.initGaussian(sigma);
        gaussBin.initGaussian(sigmaBin);
        for(size_t c=0;c<CHANNELS;++c){

            // histogram for one channel
            MultiArrayView<DIM+1,T_HIST> histc       = histogram.bindOuter(c);
            MultiArrayView<DIM+1,T_HIST> histcBuffer = histogram.bindOuter(c);

            // convolve along all spatial axis and bin axis
            // - refactor me
            if(DIM==2){
                convolveMultiArrayOneDimension(srcMultiArrayRange(histc), destMultiArray(histcBuffer), 0, gauss);
                convolveMultiArrayOneDimension(srcMultiArrayRange(histcBuffer), destMultiArray(histc), 1, gauss);
                convolveMultiArrayOneDimension(srcMultiArrayRange(histc), destMultiArray(histcBuffer), 2, gaussBin);
                histc=histcBuffer;
            }
            else if(DIM==3){
                convolveMultiArrayOneDimension(srcMultiArrayRange(histc), destMultiArray(histcBuffer), 0, gauss);
                convolveMultiArrayOneDimension(srcMultiArrayRange(histcBuffer), destMultiArray(histc), 1, gauss);
                convolveMultiArrayOneDimension(srcMultiArrayRange(histc), destMultiArray(histcBuffer), 2, gauss);
                convolveMultiArrayOneDimension(srcMultiArrayRange(histcBuffer), destMultiArray(histc), 3, gaussBin);
                histc=histcBuffer;
            }
            else if(DIM==4){
                convolveMultiArrayOneDimension(srcMultiArrayRange(histc), destMultiArray(histcBuffer), 0, gauss);
                convolveMultiArrayOneDimension(srcMultiArrayRange(histcBuffer), destMultiArray(histc), 1, gauss);
                convolveMultiArrayOneDimension(srcMultiArrayRange(histc), destMultiArray(histcBuffer), 2, gauss);
                convolveMultiArrayOneDimension(srcMultiArrayRange(histcBuffer), destMultiArray(histc), 3, gauss);
                convolveMultiArrayOneDimension(srcMultiArrayRange(histc), destMultiArray(histcBuffer), 4, gaussBin);
                histc=histcBuffer;
            }
            else{
                throw std::runtime_error("not yet implemented for arbitrary dimension");
            }
        }


    }

    template< unsigned int DIM , class T_DATA, class T_HIST >
    void multi_gaussian_co_histogram(
        const MultiArrayView<DIM, T_DATA > & imageA,
        const MultiArrayView<DIM, T_DATA > & imageB,
        const TinyVector<T_DATA,2> & minVals,
        const TinyVector<T_DATA,2> & maxVals,
        const TinyVector<int,2> & nBins,
        const TinyVector<float,3> & sigma,
        MultiArrayView<DIM+2, T_HIST> histogram
    ){
        typedef vigra::GridGraph< DIM , boost::undirected_tag> Graph;
        typedef typename Graph::NodeIt graph_scanner;
        typedef typename Graph::Node   Node;
        typedef typename MultiArrayView<DIM+2 , T_HIST>::difference_type HistCoord;
        const Graph g(imageA.shape());
        histogram.init(0.0);
        // iterate over all nodes (i.e. pixels)
        for (graph_scanner n(g); n != lemon::INVALID; ++n){

            const Node node(*n);
            T_DATA  binIndexA = imageA[node];
            T_DATA  binIndexB = imageA[node];

            binIndexA -=minVals[0];
            binIndexA /=maxVals[0];
            binIndexA *=nBins[0];

            binIndexB -=minVals[1];
            binIndexB /=maxVals[1];
            binIndexB *=nBins[1];

            HistCoord histCoord;
            for(size_t d=0;d<DIM;++d)
                histCoord[d]=node[d];
            
            histCoord[DIM]=binIndexA;
            histCoord[DIM+1]=binIndexB;

            const float fiA = binIndexA;
            const unsigned int biA = std::floor(fiA+0.5);
            const float fiB = binIndexB;
            const unsigned int biB = std::floor(fiA+0.5);
            histCoord[DIM]=std::min(biA,static_cast<unsigned int>(nBins[0]-1));
            histCoord[DIM+1]=std::min(biB,static_cast<unsigned int>(nBins[1]-1));
            histogram[histCoord]+=1.0;
            
        }

        MultiArray<DIM+2 , T_HIST>    histogramBuffer(histogram);
        Kernel1D<float> gaussS,gaussA,gaussB;
        gaussS.initGaussian(sigma[0]);
        gaussA.initGaussian(sigma[1]);
        gaussB.initGaussian(sigma[2]);

        if(DIM==2){
            convolveMultiArrayOneDimension(srcMultiArrayRange(histogram), destMultiArray(histogramBuffer), 0, gaussS);
            convolveMultiArrayOneDimension(srcMultiArrayRange(histogramBuffer), destMultiArray(histogram), 1, gaussS);
            convolveMultiArrayOneDimension(srcMultiArrayRange(histogram), destMultiArray(histogramBuffer), 2, gaussA);
            convolveMultiArrayOneDimension(srcMultiArrayRange(histogramBuffer), destMultiArray(histogram), 3, gaussB);
        }
        else if(DIM==3){
            convolveMultiArrayOneDimension(srcMultiArrayRange(histogram), destMultiArray(histogramBuffer), 0, gaussS);
            convolveMultiArrayOneDimension(srcMultiArrayRange(histogramBuffer), destMultiArray(histogram), 1, gaussS);
            convolveMultiArrayOneDimension(srcMultiArrayRange(histogram), destMultiArray(histogramBuffer), 2, gaussS);
            convolveMultiArrayOneDimension(srcMultiArrayRange(histogramBuffer), destMultiArray(histogram), 3, gaussA);
            convolveMultiArrayOneDimension(srcMultiArrayRange(histogram), destMultiArray(histogramBuffer), 4, gaussB);
            histogram=histogramBuffer;
        }
        else if(DIM==4){
            convolveMultiArrayOneDimension(srcMultiArrayRange(histogram), destMultiArray(histogramBuffer), 0, gaussS);
            convolveMultiArrayOneDimension(srcMultiArrayRange(histogramBuffer), destMultiArray(histogram), 1, gaussS);
            convolveMultiArrayOneDimension(srcMultiArrayRange(histogram), destMultiArray(histogramBuffer), 2, gaussS);
            convolveMultiArrayOneDimension(srcMultiArrayRange(histogramBuffer), destMultiArray(histogram), 3, gaussS);
            convolveMultiArrayOneDimension(srcMultiArrayRange(histogram), destMultiArray(histogramBuffer), 4, gaussA);
            convolveMultiArrayOneDimension(srcMultiArrayRange(histogramBuffer), destMultiArray(histogram), 5, gaussA);
        }
        else{
            throw std::runtime_error("not yet implemented for arbitrary dimension");
        }



    }

}
//end namespace vigra

#endif //VIGRA_MULTI_HISTOGRAMM_HXX