#include "gtest/gtest.h"

#include <memory>
#include <list>
#include <vector>

#include "carl/core/logging.h"
#include "carl/cad/CAD.h"
#include "carl/cad/Constraint.h"
#include "carl/util/platform.h"

using namespace carl;

#ifdef USE_CLN_NUMBERS
#include <cln/cln.h>
	typedef cln::cl_RA Rational;
	typedef cln::cl_I Integer;
#elif defined(__WIN)
	#pragma warning(push, 0)
	#include <mpirxx.h>
	#pragma warning(pop)
	typedef mpq_class Rational;
	typedef mpz_class Integer;
#else
	#include <gmpxx.h>
	typedef mpq_class Rational;
	typedef mpz_class Integer;
#endif

typedef carl::cad::Constraint<Rational> Constraint;

class CADTest : public ::testing::Test {
protected:
	typedef carl::CAD<Rational>::MPolynomial Polynomial;

	CADTest() : 
		x(carl::VariablePool::getInstance().getFreshVariable("x")),
		y(carl::VariablePool::getInstance().getFreshVariable("y")),
		z(carl::VariablePool::getInstance().getFreshVariable("z"))
	{
		CARL_LOG_INFO("carl.core", "Variables " << x << ", " << y);
	}
	~CADTest() {}

	virtual void SetUp() {
		// p[0] = x^2 + y^2 - 1
		this->p.push_back(Polynomial({Term<Rational>(x)*x, Term<Rational>(y)*y, Term<Rational>(-1)}));
		// p[1] = x + 1 - y
		this->p.push_back(Polynomial({Term<Rational>(x), -Term<Rational>(y), Term<Rational>(1)}));
		// p[2] = x - y
		this->p.push_back(Polynomial({Term<Rational>(x), -Term<Rational>(y)}));
		// p[3] = x^2 + y^2 + z^2 - 1
		this->p.push_back(Polynomial({Term<Rational>(x)*x, Term<Rational>(y)*y, Term<Rational>(z)*z, Term<Rational>(-1)}));
		// p[4] = x^2 + y^2
		this->p.push_back(Polynomial({Term<Rational>(x)*x, Term<Rational>(y)*y}));
		// p[5] = z^3 - 1/2
		this->p.push_back(Polynomial({Term<Rational>(z)*z*z, Term<Rational>(Rational(-1)/2)}));
	}

	virtual void TearDown() {
		//for (auto pol: this->p) delete pol;
		this->p.clear();
		this->bounds.clear();
	}

	bool hasNRValue(const carl::RealAlgebraicNumberPtr<Rational> n, Rational val) {
		if (n->isNumeric()) return n->value() == val;
		return false;
	}
	bool hasValue(const carl::RealAlgebraicNumberPtr<Rational> n, Rational val) {
		if (n->isNumeric()) return n->value() == val;
		else {
			carl::RealAlgebraicNumberIRPtr<Rational> ir = std::static_pointer_cast<carl::RealAlgebraicNumberIR<Rational>>(n);
			return ir->getInterval().contains(val);
		}
	}
	bool hasSqrtValue(const carl::RealAlgebraicNumberPtr<Rational> n, Rational val) {
		if (n->isNumeric()) return n->value() * n->value() == val;
		else {
			carl::RealAlgebraicNumberIRPtr<Rational> ir = std::static_pointer_cast<carl::RealAlgebraicNumberIR<Rational>>(n);
			return (ir->getInterval() * ir->getInterval()).contains(val);
		}
	}

	carl::CAD<Rational> cad;
	carl::Variable x, y, z;
	std::vector<Polynomial> p;
	carl::CAD<Rational>::BoundMap bounds;
};

TEST_F(CADTest, Check1)
{
	RealAlgebraicPoint<Rational> r;
	std::vector<Constraint> cons;
	this->cad.addPolynomial(this->p[0], {x, y});
	this->cad.addPolynomial(this->p[1], {x, y});
	cons.assign({
		Constraint(this->p[0], Sign::ZERO, {x,y}),
		Constraint(this->p[1], Sign::ZERO, {x,y})
	});
	this->cad.prepareElimination();
	ASSERT_TRUE(cad.check(cons, r, this->bounds));
	for (auto c: cons) ASSERT_TRUE(c.satisfiedBy(r, cad.getVariables()));
	ASSERT_TRUE((hasNRValue(r[0], -1) && hasNRValue(r[1], 0)) || (hasNRValue(r[0], 0) && hasNRValue(r[1], 1)));
	ASSERT_TRUE(cad.check(cons, r, this->bounds));
	for (auto c: cons) ASSERT_TRUE(c.satisfiedBy(r, cad.getVariables()));
	ASSERT_TRUE((hasNRValue(r[0], -1) && hasNRValue(r[1], 0)) || (hasNRValue(r[0], 0) && hasNRValue(r[1], 1)));
}

TEST_F(CADTest, Check2)
{
	RealAlgebraicPoint<Rational> r;
	std::vector<Constraint> cons;
	Rational half = Rational(1) / 2;

	this->cad.addPolynomial(this->p[0], {x, y});
	this->cad.addPolynomial(this->p[2], {x, y});
	this->cad.prepareElimination();
	cons.assign({
		Constraint(this->p[0], Sign::ZERO, {x,y}),
		Constraint(this->p[2], Sign::ZERO, {x,y})
	});
	ASSERT_TRUE(cad.check(cons, r, this->bounds));
	for (auto c: cons) ASSERT_TRUE(c.satisfiedBy(r, cad.getVariables()));
	ASSERT_TRUE((hasSqrtValue(r[0], -half) && hasSqrtValue(r[1], -half)) || (hasSqrtValue(r[0], half) && hasSqrtValue(r[1], half)));
	ASSERT_TRUE(cad.check(cons, r, this->bounds));
	for (auto c: cons) ASSERT_TRUE(c.satisfiedBy(r, cad.getVariables()));
}

TEST_F(CADTest, Check3)
{
	RealAlgebraicPoint<Rational> r;
	std::vector<Constraint> cons;

	this->cad.addPolynomial(this->p[0], {x, y});
	this->cad.addPolynomial(this->p[2], {x, y});
	this->cad.prepareElimination();
	cons.assign({
		Constraint(this->p[0], Sign::POSITIVE, {x,y}),
		Constraint(this->p[2], Sign::NEGATIVE, {x,y})
	});
	ASSERT_TRUE(cad.check(cons, r, this->bounds));
	for (auto c: cons) ASSERT_TRUE(c.satisfiedBy(r, cad.getVariables()));
}

TEST_F(CADTest, Check4)
{
	RealAlgebraicPoint<Rational> r;
	std::vector<Constraint> cons;

	this->cad.addPolynomial(this->p[0], {x, y});
	this->cad.addPolynomial(this->p[2], {x, y});
	this->cad.prepareElimination();
	cons.assign({
		Constraint(this->p[0], Sign::NEGATIVE, {x,y}),
		Constraint(this->p[2], Sign::POSITIVE, {x,y})
	});
	ASSERT_TRUE(cad.check(cons, r, this->bounds));
	for (auto c: cons) ASSERT_TRUE(c.satisfiedBy(r, cad.getVariables()));
}

TEST_F(CADTest, Check5)
{
	RealAlgebraicPoint<Rational> r;
	std::vector<Constraint> cons;

	this->cad.addPolynomial(this->p[0], {x, y});
	this->cad.addPolynomial(this->p[2], {x, y});
	this->cad.prepareElimination();
	cons.assign({
		Constraint(this->p[0], Sign::ZERO, {x,y}),
		Constraint(this->p[2], Sign::POSITIVE, {x,y})
	});
	ASSERT_TRUE(cad.check(cons, r, this->bounds));
	for (auto c: cons) ASSERT_TRUE(c.satisfiedBy(r, cad.getVariables()));
}

TEST_F(CADTest, Check6)
{
	RealAlgebraicPoint<Rational> r;
	std::vector<Constraint> cons;

	this->cad.addPolynomial(this->p[3], {x, y, z});
	this->cad.addPolynomial(this->p[4], {x, y, z});
	this->cad.addPolynomial(this->p[5], {x, y, z});
	this->cad.prepareElimination();
	cons.assign({
		Constraint(this->p[3], Sign::NEGATIVE, {x,y,z}),
		Constraint(this->p[4], Sign::POSITIVE, {x,y,z}),
		Constraint(this->p[5], Sign::POSITIVE, {x,y,z})
	});
	ASSERT_TRUE(cad.check(cons, r, this->bounds));
	for (auto c: cons) ASSERT_TRUE(c.satisfiedBy(r, cad.getVariables()));
}

template<typename T>
inline std::shared_ptr<carl::RealAlgebraicNumberNR<Rational>> NR(T t, bool b) {
	return carl::RealAlgebraicNumberNR<Rational>::create(t, b);
}

TEST(CAD, Samples)
{
	std::list<carl::RealAlgebraicNumberPtr<Rational>> roots({ NR(-1, true), NR(1, true) });
	
	carl::cad::SampleSet<Rational> currentSamples;
	currentSamples.insert(NR(-1, false));
	currentSamples.insert(NR(0, true));
	currentSamples.insert(NR(1, false));
	
	std::forward_list<carl::RealAlgebraicNumberPtr<Rational>> replacedSamples;
	
	carl::Interval<Rational> bounds(0, carl::BoundType::STRICT, 1, carl::BoundType::INFTY);
	
	carl::CAD<Rational> cad;
	
	//carl::cad::SampleSet<Rational> res = cad.samples(0, roots, currentSamples, replacedSamples, bounds);

	//std::cout << res << std::endl;
	//ASSERT_TRUE(!res.empty());
}
