context("lotri -- easy matrix parsing");
test_that("lotri matrix parsing", {
    expect_equal(lotri({et2 + et3 + et4 ~ c(40,
                                            0.1, 20,
                                            0.1, 0.1, 30)}),
                 structure(c(40, 0.1, 0.1, 0.1, 20, 0.1, 0.1, 0.1, 30),
                           .Dim = c(3L, 3L),
                           .Dimnames = list(c("et2", "et3", "et4"),
                                            c("et2", "et3", "et4"))))

    expect_equal(lotri(list(et2 + et3 + et4 ~ c(40,
                                                0.1, 20,
                                                0.1, 0.1, 30),
                            matrix(1,dimnames=list("et5","et5")))),
                 structure(c(40, 0.1, 0.1, 0, 0.1, 20, 0.1, 0, 0.1, 0.1, 30, 0,
                             0, 0, 0, 1),
                           .Dim = c(4L, 4L),
                           .Dimnames = list(c("et2", "et3", "et4", "et5"),
                                            c("et2", "et3", "et4", "et5"))))

    expect_equal(lotri(list(et2 + et3 + et4 ~ c(40,
                                                0.1, 20,
                                                0.1, 0.1, 30),
                            matrix(1,dimnames=list("et5","et5")))),
                 structure(c(40, 0.1, 0.1, 0, 0.1, 20, 0.1, 0, 0.1, 0.1, 30, 0,
                             0, 0, 0, 1),
                           .Dim = c(4L, 4L),
                           .Dimnames = list(c("et2", "et3", "et4", "et5"),
                                            c("et2", "et3", "et4", "et5"))))

    expect_equal(lotri({
        et2 + et3 + et4 ~ c(40,
                            0.1, 20,
                            0.1, 0.1, 30);
        et5 ~ 1;}),
        structure(c(40, 0.1, 0.1, 0, 0.1, 20, 0.1, 0, 0.1, 0.1, 30, 0,
                    0, 0, 0, 1),
                  .Dim = c(4L, 4L),
                  .Dimnames = list(c("et2", "et3", "et4", "et5"),
                                   c("et2", "et3", "et4", "et5"))))
    expect_equal(lotri(et2 + et3 + et4 ~ c(40,
                                           0.1, 20,
                                           0.1, 0.1, 30),
                       et5 ~ 1),
                 structure(c(40, 0.1, 0.1, 0, 0.1, 20, 0.1, 0, 0.1, 0.1, 30, 0,
                             0, 0, 0, 1),
                           .Dim = c(4L, 4L),
                           .Dimnames = list(c("et2", "et3", "et4", "et5"),
                                            c("et2", "et3", "et4", "et5"))))

    expect_equal(lotri(et2 + et3 + et4 ~ c(40,
                                           0.1, 20,
                                           0.1, 0.1, 30),
                       list(et5 ~ 1, et6 ~ 3)),
                 structure(c(40, 0.1, 0.1, 0, 0, 0.1, 20, 0.1, 0, 0, 0.1, 0.1,
                             30, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 3),
                           .Dim = c(5L, 5L),
                           .Dimnames = list(c("et2", "et3", "et4", "et5", "et6"),
                                            c("et2", "et3", "et4", "et5", "et6"))))
    expect_equal(lotri(quote({et2 + et3 + et4 ~ c(40,
                                                  0.1, 20,
                                                  0.1, 0.1, 30)})),
                 structure(c(40, 0.1, 0.1, 0.1, 20, 0.1, 0.1, 0.1, 30),
                           .Dim = c(3L, 3L),
                           .Dimnames = list(c("et2", "et3", "et4"),
                                            c("et2", "et3", "et4"))))

    .mat  <- lotri({et2 + et3 + et4 ~ c(40,
                                        0.1, 20,
                                        0.1, 0.1, 30)})
    ## Test for NSE issues
    expect_equal(.mat, lotri(.mat))
    ## Test for NULL
    expect_equal(NULL, lotri(NULL))

    expect_equal(lotri(eta.Cl ~ 0.4^2), structure(0.16, .Dim = c(1L, 1L), .Dimnames = list("eta.Cl",
    "eta.Cl")))

    ## Parsing errors
    expect_error(lotri(a~fix(3)))
    expect_error(lotri(a~c(3,1,3)))
    expect_error(lotri(a~c(3,1)))
    expect_error(lotri({a=c(3,1)}))

    expect_equal(lotri({matrix(3, dimnames=list("a","a"))}),
                 structure(3, .Dim = c(1L, 1L), .Dimnames = list("a", "a")))

    expect_equal(lotri(matrix(3, dimnames=list("a","a"))),
                 structure(3, .Dim = c(1L, 1L), .Dimnames = list("a", "a")))

    expect_error(lotri(quote(matrix(3, dimnames=list("a","a")))))

    expect_error(lotri(lotri(a ~ paste(1))))

})