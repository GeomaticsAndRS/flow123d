#!/usr/bin/python
# -*- coding: utf-8 -*-
# author:   Jan Hybs
# ----------------------------------------------
from scripts.core.base import Paths
# ----------------------------------------------


class CompareNdiff(object):
    @staticmethod
    def get_command(f1, f2, **details):
        return [
            Paths.ndiff(),
            '-r', str(details.get('r_tol', '0.001')),
            '-a', str(details.get('a_tol', '0.000001')),
            Paths.abspath(f1),
            Paths.abspath(f2)
        ]