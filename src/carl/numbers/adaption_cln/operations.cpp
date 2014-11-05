#include "operations.h"

namespace carl
{
    template<>
    cln::cl_RA rationalize<cln::cl_RA>(double n) {
        switch (std::fpclassify(n)) {
            case FP_NORMAL: // normalized are fully supported
                return cln::rationalize(cln::cl_R(n));
            case FP_SUBNORMAL: { // subnormals result in underflows, hence the value of the double is 0.f, where f is the significand precision
                assert(sizeof(n) == 8);
                long int significandBits = reinterpret_cast<long int>(&n);
                significandBits = (significandBits << 12) >> 12;
                if( n < 0 )
                    significandBits = -significandBits;
                return cln::cl_RA( significandBits ) * ONE_DIVIDED_BY_10_TO_THE_POWER_OF_52;
        }
        case FP_ZERO:
            return 0;
        case FP_NAN: // NaN and infinite are not supported
        case FP_INFINITE:
            assert(false);
            break;
        }
        return 0;
    }
    
    template<>
    cln::cl_RA rationalize<cln::cl_RA>(float n)
    {
        switch (std::fpclassify(n))
        {
            case FP_NORMAL: // normalized are fully supported
                return cln::rationalize(cln::cl_R(n));
            case FP_SUBNORMAL: { // subnormals result in underflows, hence the value of the double is 0.f, where f is the significand precision
                assert(sizeof(n) == 4);
                long int significandBits = reinterpret_cast<long int>(&n);
                significandBits = (significandBits << 9) >> 9;
                if( n < 0 )
                    significandBits = -significandBits;
                return cln::cl_RA( significandBits ) * ONE_DIVIDED_BY_10_TO_THE_POWER_OF_23;
        }
        case FP_ZERO:
            return 0;
        case FP_NAN: // NaN and infinite are not supported
        case FP_INFINITE:
            assert(false);
            break;
        }
        return 0;
    }
    
    std::pair<cln::cl_RA, cln::cl_RA> sqrt(const cln::cl_RA& a)
    {
        assert( a >= 0 );
        cln::cl_R root = cln::sqrt(toLF(a));
        cln::cl_RA rroot = cln::rationalize(root);
        if( rroot == root ) // the result of the sqrt operation is a rational and thus an exact solution -> return a point-Interval
        {
            return std::make_pair(rroot, rroot);
        }
        else // we need to find the second bound of the overapprox. - the first is given by the rationalized result.
        {
                    // Check if root^2 > a
            if( cln::expt_pos(rroot,2) > a ) // we need to find the lower bound
            {
                cln::cl_R lower = cln::sqrt(toLF(a-rroot));
                cln::cl_RA rlower = cln::rationalize(lower);
                if( rlower == lower )
                {
                    return std::make_pair(rlower, rroot);
                }
                else
                {
                    cln::cl_I num = cln::numerator(rlower);
                    cln::cl_I den = cln::denominator(rlower);
                    --num;
                    return std::make_pair( num/den, rroot );
                }
            }
            else // we need to find the upper bound
            {
                cln::cl_R upper = cln::sqrt(toLF(a+rroot));
                cln::cl_RA rupper = cln::rationalize(upper);
                if( rupper == upper )
                {
                    return std::make_pair(rroot, rupper);
                }
                else
                {
                    cln::cl_I num = cln::numerator(rupper);
                    cln::cl_I den = cln::denominator(rupper);
                    ++num;
                    return std::make_pair(rroot, num/den );
                }
            }
        }
    }
    
    std::pair<cln::cl_RA, cln::cl_RA> sqrt_fast(const cln::cl_RA& a)
    {
	assert(a >= 0);
	cln::cl_R tmp = cln::sqrt(toLF(a));
	cln::cl_RA root = cln::rationalize(tmp);
	if(root == tmp) {
            // root is a cln::cl_RA
            return std::make_pair(root, root);
	} else {
            // root is a cln::cl_LF. In this case, it is not integer and we can assume that the surrounding integers contain the actual root.
            cln::cl_I lower = carl::floor(root);
            cln::cl_I upper = carl::ceil(root);
            assert(cln::expt_pos(lower,2) < a);
            assert(cln::expt_pos(upper,2) > a);
            return std::make_pair(lower, upper);
        }
    }
    
    template<>
    cln::cl_RA rationalize<cln::cl_RA>(const std::string& inputstring)
    {
	std::vector<std::string> strs;
        boost::split(strs, inputstring, boost::is_any_of("."));

        if(strs.size() > 2)
        {
            throw std::invalid_argument("More than one delimiter in the string.");
        }
        cln::cl_RA result(0);
        if(!strs.front().empty())
        {
            result += cln::cl_RA(strs.front().c_str());
        }
        if(strs.size() > 1)
        {
            result += (cln::cl_RA(strs.back().c_str())/carl::pow(cln::cl_I(10),static_cast<unsigned>(strs.back().size())));
        }
        return result;
    }

    std::string toString(const cln::cl_RA& _number, bool _infix)
    {
        std::stringstream s;
        bool negative = (_number < cln::cl_RA(0));
        if(negative) s << "(-" << (_infix ? "" : " ");
        if(_infix) s << carl::abs(_number);
        else
        {
            cln::cl_I d = carl::getDenom(_number);
            if(d != cln::cl_I(1)) s << "(/ " << carl::abs(carl::getNum(_number)) << " " << carl::abs(d) << ")";
            else s << carl::abs(_number);
        }
        if(negative) 
            s << ")";
        return s.str();
    }

    std::string toString(const cln::cl_I& _number, bool _infix)
    {
        std::stringstream s;
        bool negative = (_number < cln::cl_I(0));
        if(negative) s << "(-" << (_infix ? "" : " ");
        s << carl::abs(_number);
        if(negative) 
            s << ")";
        return s.str();
    }
}