namespace cpp glipf

struct Dims {
  1: double x,
  2: double y,
  3: double z
}
struct Point3d {
  1: double x,
  2: double y,
  3: double z
}

struct Target {
  1: i32 id,
  2: Point3d pose
}

struct Particle {
  1: i32 id,
  2: Point3d pose,
}


service GlipfServer {

  void initForegroundCoverageProcessor(1: list<Point3d> modelCenters,
                                       2: Dims modelDims)
  list<double> scanForeground()
  void initTarget(1: Target targetData, 2: bool computeRef)
  bool isVisible(1: list<Target> targets, 2: Target newTarget)
  void targetUpdate(1: list<Target> targets)
  list<double> computeDistance(1: list<Particle> particles)
  void drawDebugOutput(1: list<Target> targets, 2: bool drawParticles)
  void grabFrame()
}
