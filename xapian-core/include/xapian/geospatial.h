/** @file geospatial.h
 * @brief Geospatial search support routines.
 */
/* Copyright 2008,2009 Lemur Consulting Ltd
 * Copyright 2010 Richard Boulton
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA
 */

#ifndef XAPIAN_INCLUDED_GEOSPATIAL_H
#define XAPIAN_INCLUDED_GEOSPATIAL_H

#include <xapian/enquire.h>
#include <xapian/postingsource.h>
#include <xapian/queryparser.h> // For sortable_serialise
#include <xapian/keymaker.h>
#include <xapian/visibility.h>
#include <string>
#include <set>

namespace Xapian {

class Registry;

/** Convert from miles to metres.
 */
inline XAPIAN_VISIBILITY_DEFAULT double
miles_to_metres(double miles)
{
    return 1609.344 * miles;
}

/** Convert from metres to miles.
 */
inline XAPIAN_VISIBILITY_DEFAULT double
metres_to_miles(double metres)
{
    return metres * (1.0 / 1609.344);
}

/** A latitude-longitude coordinate.
 *
 *  Note that latitude-longitude coordinates are only precisely meaningful if
 *  the datum used to define them is specified.  This class ignores this
 *  issue - it is up to the caller to ensure that the datum used for each
 *  coordinate in a system is consistent.
 */
struct XAPIAN_VISIBILITY_DEFAULT LatLongCoord {
  public:
    /** A latitude, as decimal degrees.
     *
     *  Should be in the range -90 <= longitude <= 90
     *
     *  Postive latitudes represent the northern hemisphere.
     */
    double latitude;

    /** A longitude, as decimal degrees.
     *
     *  Should be in the range -180 < latitude <= 180
     *
     *  Positive longitudes represent the eastern hemisphere.
     */
    double longitude;

    /** Construct a coordinate.
     *
     *  If the supplied longitude is out of range, an exception will be raised.
     *
     *  If the supplied latitude is out of range, it will be normalised to the
     *  appropriate range.
     */
    LatLongCoord(double latitude_, double longitude_);

    /** Construct a coordinate by unserialising a string.
     *
     *  @param serialised the string to unserialise the coordinate from.
     *
     *  @exception Xapian::SerialisationError if the string does not contain
     *  a valid serialised latitude-longitude pair, or contains extra data at
     *  the end of it.
     */
    static LatLongCoord unserialise(const std::string & serialised);

    /** Construct a coordinate by unserialising a string.
     *
     *  The string may contain further data after that for the coordinate.
     *
     *  @param ptr A pointer to the start of the string.  This will be updated
     *  to point to the end of the data representing the coordinate.
     *  @param end A pointer to the end of the string.
     *
     *  @exception Xapian::SerialisationError if the string does not contain
     *  a valid serialised latitude-longitude pair.
     */
    static LatLongCoord unserialise(const char ** ptr, const char * end);

    /** Return a serialised representation of the coordinate.
     */
    std::string serialise() const;

    /** Compare with another LatLongCoord.
     */
    bool operator<(const LatLongCoord & other) const
    {
	if (latitude < other.latitude) return true;
	return (longitude < other.longitude);
    }

    /// Return a string describing this object.
    std::string get_description() const;
};

/** A set of latitude-longitude coordinate.
 */
class XAPIAN_VISIBILITY_DEFAULT LatLongCoords {
    /// The coordinates.
    std::set<LatLongCoord> coords;

  public:
    std::set<LatLongCoord>::const_iterator begin() const
    {
	return coords.begin();
    }

    std::set<LatLongCoord>::const_iterator end() const
    {
	return coords.end();
    }

    size_t size() const
    {
	return coords.size();
    }

    size_t empty() const
    {
	return coords.empty();
    }

    void insert(const LatLongCoord & coord)
    {
	coords.insert(coord);
    }

    void erase(const LatLongCoord & coord)
    {
	coords.erase(coord);
    }

    /// Construct an empty set of coordinates.
    LatLongCoords() : coords() {}

    /// Construct a set of coordinates containing one coordinate.
    LatLongCoords(const LatLongCoord & coord) : coords()
    {
	coords.insert(coord);
    }

    /** Construct a set of coordinates by unserialising a string.
     *
     *  @param serialised the string to unserialise the coordinates from.
     *
     *  @exception Xapian::SerialisationError if the string does not contain
     *  a valid serialised latitude-longitude pair, or contains junk at the end
     *  of it.
     */
    static LatLongCoords unserialise(const std::string & serialised);

    /** Return a serialised form of the coordinate list.
     */
    std::string serialise() const;

    /// Return a string describing this object.
    std::string get_description() const;
};

/** Base class for calculating distances between two lat/long coordinates.
 */
class XAPIAN_VISIBILITY_DEFAULT LatLongMetric {
  public:
    /// Destructor.
    virtual ~LatLongMetric();

    /** Return the distance between two coordinates, in metres.
     */
    virtual double operator()(const LatLongCoord & a, const LatLongCoord &b) const = 0;

    /** Return the distance between two coordinate lists, in metres.
     *
     *  The distance between the coordinate lists is defined to the be minimum
     *  pairwise distance between coordinates in the lists.
     *
     *  If either of the lists is empty, an InvalidArgumentError will be raised.
     */
    double operator()(const LatLongCoords & a, const LatLongCoords &b) const;

    /** Clone the metric. */
    virtual LatLongMetric * clone() const = 0;

    /** Return the full name of the metric.
     *
     *  This is used when serialising and unserialising metrics; for example,
     *  for performing remote searches.
     *
     *  If the subclass is in a C++ namespace, the namespace should be included
     *  in the name, using "::" as a separator.  For example, for a
     *  LatLongMetric subclass called "FooLatLongMetric" in the "Xapian"
     *  namespace the result of this call should be "Xapian::FooLatLongMetric".
     */
    virtual std::string name() const = 0;

    /** Serialise object parameters into a string.
     *
     *  The serialised parameters should represent the configuration of the
     *  metric.
     */
    virtual std::string serialise() const = 0;

    /** Create object given string serialisation returned by serialise().
     *
     *  @param s A serialised instance of this LatLongMetric subclass.
     */
    virtual LatLongMetric * unserialise(const std::string & s) const = 0;
};

/** Calculate the great-circle distance between two coordinates on a sphere.
 *
 *  This uses the haversine formula to calculate the distance.  Note that this
 *  formula is subject to inaccuracy due to numerical errors for coordinates on
 *  the opposite side of the sphere.
 *
 *  See http://en.wikipedia.org/wiki/Haversine_formula
 */
class XAPIAN_VISIBILITY_DEFAULT GreatCircleMetric : public LatLongMetric {
    /** The radius of the sphere in metres.
     */
    double radius;

  public:
    /** Construct a GreatCircleMetric.
     *
     *  The (quadratic mean) radius of the earth will be used by this
     *  calculator.
     */
    GreatCircleMetric();

    /** Construct a GreatCircleMetric using a specified radius.
     *
     *  @param radius_ The radius of to use, in metres.
     */
    GreatCircleMetric(double radius_);

    /** Return the great-circle distance between points on the sphere.
     */
    double operator()(const LatLongCoord & a, const LatLongCoord &b) const;

    LatLongMetric * clone() const;
    std::string name() const;
    std::string serialise() const;
    LatLongMetric * unserialise(const std::string & s) const;
};

/** Posting source which returns a weight based on geospatial distance.
 *
 *  Results are weighted by the distance from a fixed point, or list of points,
 *  calculated according to the metric supplied.  If multiple points are
 *  supplied (either in the constructor, or in the coordinates stored in a
 *  document) , the closest pointwise distance is returned.
 *
 *  Documents further away than a specified maximum range (or with no location
 *  stored in the specified slot) will not be returned.
 *
 *  The weight returned will be computed from the distance using the formula:
 *  k1 * (distance + k1) ** (- k2)
 *
 *  (Where k1 and k2 are (strictly) positive, floating point, constants, and
 *  default to 1000 and 1, respectively.  Distance is measured in metres, so
 *  this means that something at the centre gets a weight of 1.0, something 1km
 *  away gets a weight of 0.5, and something 3km away gets a weight of 0.25,
 *  etc)
 */
class XAPIAN_VISIBILITY_DEFAULT LatLongDistancePostingSource : public ValuePostingSource
{
    /// Current distance from centre.
    double dist;

    /// Centre, to compute distance from.
    LatLongCoords centre;

    /// Metric to compute the distance with.
    const LatLongMetric * metric;

    /// Maximum range to allow.  If set to 0, there is no maximum range.
    double max_range;

    /// Constant used in weighting function.
    double k1;

    /// Constant used in weighting function.
    double k2;

    /** Calculate the distance for the current document.
     *
     *  Returns true if the distance was calculated ok, or false if the
     *  document didn't contain a valid serialised set of coordinates in the
     *  appropriate value slot.
     */
    void calc_distance();

    /// Internal constructor; used by clone() and serialise().
    LatLongDistancePostingSource(Xapian::valueno slot_,
				 const LatLongCoords & centre_,
				 const LatLongMetric * metric_,
				 double max_range_,
				 double k1_,
				 double k2_);

  public:
    /** Construct a new match decider which returns only documents within
     *  range of one of the central coordinates.
     *
     *  @param db_ The database to read values from.
     *  @param slot_ The value slot to read values from.
     *  @param centre_ The centre point to use for distance calculations.
     *  @param metric_ The metric to use for distance calculations.
     *  @param max_range_ The maximum distance for documents which are returned.
     *  @param k1_ The k1 constant to use in the weighting function.
     *  @param k2_ The k2 constant to use in the weighting function.
     */
    LatLongDistancePostingSource(Xapian::valueno slot_,
				 const LatLongCoords & centre_,
				 const LatLongMetric & metric_,
				 double max_range_ = 0.0,
				 double k1_ = 1000.0,
				 double k2_ = 1.0);
    ~LatLongDistancePostingSource();

    void next(Xapian::weight min_wt);
    void skip_to(Xapian::docid min_docid, Xapian::weight min_wt);
    bool check(Xapian::docid min_docid, Xapian::weight min_wt);

    Xapian::weight get_weight() const;
    LatLongDistancePostingSource * clone() const;
    std::string name() const;
    std::string serialise() const;
    LatLongDistancePostingSource *
	    unserialise_with_registry(const std::string &s,
				      const Registry & registry) const;
    void init(const Database & db_);

    std::string get_description() const;
};

/** KeyMaker subclass which sorts by distance from a latitude/longitude.
 *
 *  Results are ordered by the distance from a fixed point, or list of points,
 *  calculated according to the metric supplied.  If multiple points are
 *  supplied (either in the constructor, or in the coordinates stored in a
 *  document), the closest pointwise distance is returned.
 *
 *  If a document contains no
 */
class XAPIAN_VISIBILITY_DEFAULT LatLongDistanceKeyMaker : public KeyMaker {

    /// The value slot to read.
    Xapian::valueno valno;

    /// The centre point (or points) for distance calculation.
    LatLongCoords centre;

    /// The metric to use when calculating distances.
    const LatLongMetric * metric;

    /// The default key to return, for documents with no value stored.
    std::string defkey;

  public:
    LatLongDistanceKeyMaker(Xapian::valueno valno_,
			    const LatLongCoords & centre_,
			    const LatLongMetric & metric_,
			    double defdistance = 10E10)
	    : valno(valno_),
	      centre(centre_),
	      metric(metric_.clone()),
	      defkey(sortable_serialise(defdistance))
    {}

    LatLongDistanceKeyMaker(Xapian::valueno valno_,
			    const LatLongCoord & centre_,
			    const LatLongMetric & metric_,
			    double defdistance = 10E10)
	    : valno(valno_),
	      centre(),
	      metric(metric_.clone()),
	      defkey(sortable_serialise(defdistance))
    {
	centre.insert(centre_);
    }

    ~LatLongDistanceKeyMaker();

    std::string operator()(const Xapian::Document & doc) const;
};

}

#endif /* XAPIAN_INCLUDED_GEOSPATIAL_H */