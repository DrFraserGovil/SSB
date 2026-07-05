# Changelog

<!-- All notable changes to this project will be documented in this file. -->

The format is based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0/)
and this project adheres to [Semantic Versioning](http://semver.org/spec/v2.0.0.html).

TODO: Strip out the old code, salvaging what can be salvaged
TODO: Rename? SSB isn't quite right any more - it's not serialised in any meaningful way
TODO: Create a lightweight parser that can read files & detect a JSL::Interface usage
TODO: Comment detector + parser
TODO: Rewriter; in place + _automated file
 
## [Unreleased] 


### Added

* More sophisticated build system; integration with FetchContent to automatically configure against a chosen JSL version
* A documentation system for explaining both the syntax, and the workings of the code
 
### Changed
 
* Renamed the module to reflect its new purpose; no longer is it a `Serialised Settings Builder', it is instead a reflection-generator; and so the name LSJ has stuck
<!-- ### Removed -->


