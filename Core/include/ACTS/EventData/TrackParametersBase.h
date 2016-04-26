#ifndef ACTS_PARAMETERSBASE_H
#define ACTS_PARAMETERSBASE_H 1

// STL include(s)
#include <type_traits>

#include "ACTS/Utilities/Definitions.h"
// ACTS includes
#include "ACTS/Utilities/ParameterDefinitions.h"
#include "ACTS/EventData/ParameterSet.h"
#include "ACTS/EventData/ChargePolicy.h"

namespace Acts
{
  class Surface;

  /**
     @class ParametersBase

     The base class for neutral and charged Track parameters.
     It represents the free state of a trajectory, represented by
     the track parameters.
     The position and the momentum are both given in the tracking
     reference frame.

     @tparam DIM number of track parameters (usually 5)
     @tparam T   charge of track (either <tt>Charged</tt> or <tt>Neutral</tt>)

     @author Andreas.Salzburger@cern.ch
  */

  class TrackParametersBase
  {
  public:
    typedef ActsVector<ParValue_t,Acts::NGlobalPars> ParVector_t;
    typedef ActsSymMatrix<ParValue_t,Acts::NGlobalPars> CovMatrix_t;

    /** virtual destructor */
    virtual ~TrackParametersBase() = default;

    virtual TrackParametersBase* clone() const = 0;

    //** equality operator */
    virtual bool operator==(const TrackParametersBase& rhs) const = 0;

    //** inequality operator */
    bool operator!=(const TrackParametersBase& rhs) const
    {
      return !(*this == rhs);
    }

    /** Access method for the position */
    virtual ActsVectorD<3> position() const = 0;

    /** Access method for the momentum */
    virtual ActsVectorD<3> momentum() const = 0;

    /** Access method for the parameters */
    ParVector_t parameters() const
    {
      return getParameterSet().getParameters();
    }

    template<ParID_t par>
    ParValue_t get() const
    {
      return getParameterSet().template getParameter<par>();
    }

    /** Access method for the covariance matrix - returns 0 if no covariance matrix is given */
    const CovMatrix_t* covariance() const
    {
      return getParameterSet().getCovariance();
    }

    /** Access method for transverse momentum */
    double pT() const {return momentum().perp();}

    /** Access method for pseudorapidity - from momentum */
    double eta() const {return momentum().eta();}

    /** Dumps relevant information about the track parameters into the ostream */
    std::ostream& dump(std::ostream& out) const;

    /** Returns charge of concrete type (i.e. must be implemented in inheriting classes) */
    virtual double charge() const = 0;

    virtual const Surface& associatedSurface() const = 0;

  protected:
    virtual FullParameterSet& getParameterSet() = 0;
    virtual const FullParameterSet& getParameterSet() const = 0;
  };

  std::ostream& operator<<(std::ostream& sl,const TrackParametersBase& tp);

} // end of namespace Acts

#endif // ACTS_PARAMETERSBASE_h