import unittest
from cruise_pid import car 


class TestCruisePid(unittest.TestCase):
    car_inst = car(x=[0])

    def test_force_vehicle(self):
        result = self.car_inst.force_vehicle(
            x=[15], u=[0.3], params={'ratedPower': 40e3})
        self.assertEqual(result, 0)
        result = self.car_inst.force_vehicle(
            x=[50], u=[1], params={'ratedPower': 40e3})
        self.assertEqual(result, 40e3/(50/3.6))
        result = self.car_inst.force_vehicle(
            x=[51], u=[0], params={'ratedPower': 40e3})
        self.assertEqual(result, -0.5*40e3/(51/3.6))
        result = self.car_inst.force_vehicle(
            x=[16], u=[0], params={'ratedPower': 40e3})
        self.assertEqual(result, -0.5*40e3*16/50/(16/3.6))


if __name__ == '__main__':
    unittest.main()
