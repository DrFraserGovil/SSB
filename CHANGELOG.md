# Changelog

<!-- All notable changes to this project will be documented in this file. -->

The format is based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0/)
and this project adheres to [Semantic Versioning](http://semver.org/spec/v2.0.0.html).

## [Unreleased]

* Ensures the autogen files declare their contents to be public (a prerequisite for detection as a valid aggregator)


## [1.0.0] 2026-07-07 

This is a complete overhaul of the pre-release versions that were attempted. 1.0.0 is a complete rewrite of the original test scripts.

* Added robust install and syntax guidelines to the readme
* In-place reflection generator with #inlcude-s to the ``.autogen`` files which contain the necessary 
* Robust parsing of a limited 'sane' subset of C++
* Comment parsing for metadata allocation
* Renamed the module to reflect its new purpose; no longer is it a 'Serialised Settings Builder', it is instead a reflection-generator; and so the name LSJ has stuck

