# Gerrymandering and Contiguous District Generation (Exploratory Project)

In this project I explore algorithmic approaches to generating **contiguous electoral districts** from county-level data, motivated by questions around gerrymandering, spatial constraints, and emergent political structure. The code implements a greedy algorithm on a county adjacency graph, with the goal of constructing districts that are contiguous and approximately population-balanced.

This repo represents an **exploratory research project**, not a tool that is ready for practical use -- see 'Limitations' for more.

---

## Overview

The core idea of the project is to model counties as nodes in a graph, with edges representing geographic adjacency, and to grow districts outward from a set of initial “seed” counties. Each district grows by iteratively absorbing neighboring counties that are most similar in a chosen feature space (e.g. demographics, wages, food access), while respecting population constraints and contiguity.

At a high level, the algorithm proceeds as follows:

1. Load county-level demographic and economic data.
2. Construct an adjacency graph between counties.
3. Select initial seed counties intended to be diverse in feature space.
4. Grow districts greedily by adding neighboring counties until population targets are reached.
5. Repeat with different seeds if constraints cannot be satisfied.

---

## What Is Implemented

- Parsing of county demographic and economic data from CSV files.
- Construction of a county adjacency graph.
- A greedy, priority-queue–based district-growing algorithm.
- Multiple restart attempts with different seed selections.
- Distance calculations in feature space to guide district growth.

---

## Known Limitations

This project intentionally documents its limitations, which are typical of early-stage research code:

- **Greedy failure modes:** the region-growing approach can exhaust valid moves before all counties are assigned, especially under strict population constraints.
- **Sensitivity to normalization:** distance calculations depend strongly on feature scaling; improper normalization can destabilize the algorithm.
- **No repair phase:** there is currently no post-processing step to fix boundary issues or reassign counties when greedy growth fails.
- **Seed selection ignores geography:** initial seeds are chosen based on feature diversity rather than graph distance, which can lead to difficult-to-grow configurations.
- **Incomplete robustness:** some real-world data inconsistencies (e.g. mismatched county names across datasets) require additional validation checks.

As a result, the algorithm does **not** reliably produce valid districting plans in all cases, and output quality varies by dataset and parameters.

---

## Why This Is Still Useful

Despite its limitations, this project demonstrates:

- How spatial and political constraints can be formalized algorithmically.
- How local, greedy rules can lead to global failures in constrained systems.
- The difficulty of satisfying contiguity, balance, and fairness simultaneously.
- The importance of backtracking, repair, or stochastic methods in redistricting problems.

These challenges are not bugs so much as **features of the problem itself**, and understanding them was a central learning outcome of the project.

---

## Status

**Exploratory / In progress.**

